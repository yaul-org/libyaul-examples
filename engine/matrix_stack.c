/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdbool.h>
#include <stdio.h>

#include "matrix_stack.h"

SLIST_HEAD(matrix_stacks, matrix_stack);

MEMB(matrix_stack_pool, struct matrix_stack, MATRIX_STACK_DEPTH,
    sizeof(struct matrix_stack));
MEMB(matrix_stack_matrix_pool, fix16_matrix3_t, MATRIX_STACK_DEPTH,
    sizeof(fix16_matrix3_t));

static struct {
        bool initialized;
        int32_t mode;

        struct matrix_stacks matrix_stacks[MATRIX_STACK_MODES];
} _state;

void
matrix_stack_init(void)
{
        if (_state.initialized) {
                return;
        }

        SLIST_INIT(&_state.matrix_stacks[MATRIX_STACK_MODE_PROJECTION]);
        SLIST_INIT(&_state.matrix_stacks[MATRIX_STACK_MODE_MODEL_VIEW]);

        memb_init(&matrix_stack_pool);
        memb_init(&matrix_stack_matrix_pool);

        _state.initialized = true;
        _state.mode = MATRIX_STACK_MODE_INVALID;

        matrix_stack_mode(MATRIX_STACK_MODE_PROJECTION);
        matrix_stack_push();
        matrix_stack_identity_load();

        matrix_stack_mode(MATRIX_STACK_MODE_MODEL_VIEW);
        matrix_stack_push();
        matrix_stack_identity_load();
}

void
matrix_stack_mode(int32_t mode)
{
        assert(_state.initialized);
        assert((mode == MATRIX_STACK_MODE_PROJECTION) ||
               (mode == MATRIX_STACK_MODE_MODEL_VIEW));

        _state.mode = mode;
}

void
matrix_stack_push(void)
{
        /* Make sure the correct state is set */
        assert(_state.initialized);
        assert(_state.mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *ms;
        ms = (struct matrix_stack *)memb_alloc(&matrix_stack_pool);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_state.mode);

        ms->ms_matrix = (fix16_matrix3_t *)memb_alloc(
                &matrix_stack_matrix_pool);
        assert(ms->ms_matrix != NULL);

        if (top_ms != NULL) {
                memcpy(ms->ms_matrix, top_ms->ms_matrix,
                    sizeof(fix16_matrix3_t));
        }

        SLIST_INSERT_HEAD(&_state.matrix_stacks[_state.mode], ms,
            ms_entries);
}

void
matrix_stack_pop(void)
{
        /* Make sure the correct state is set */
        assert(_state.initialized);
        assert(_state.mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_state.mode);
        assert(top_ms != NULL);

        int error_code;
        error_code = memb_free(&matrix_stack_matrix_pool, top_ms->ms_matrix);
        assert(error_code == 0);
        error_code = memb_free(&matrix_stack_pool, top_ms);
        assert(error_code == 0);
        SLIST_REMOVE_HEAD(&_state.matrix_stacks[_state.mode],
            ms_entries);

        /* Make sure we didn't pop off the last matrix in the stack */
        assert(!(SLIST_EMPTY(&_state.matrix_stacks[_state.mode])));
}

struct matrix_stack *
matrix_stack_top(int32_t mode)
{
        /* Make sure the correct state is set */
        assert(_state.initialized);
        assert((mode == MATRIX_STACK_MODE_PROJECTION) ||
               (mode == MATRIX_STACK_MODE_MODEL_VIEW));

        return SLIST_FIRST(&_state.matrix_stacks[mode]);
}

void
matrix_stack_load(fix16_matrix3_t *matrix)
{
        /* Make sure the correct state is set */
        assert(_state.initialized);
        assert(_state.mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_state.mode);
        assert(top_ms != NULL);

        memcpy(top_ms->ms_matrix, matrix, sizeof(fix16_matrix3_t));
}

void
matrix_stack_identity_load(void)
{
        /* Make sure the correct state is set */
        assert(_state.initialized);
        assert(_state.mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_state.mode);
        assert(top_ms != NULL);

        fix16_matrix3_identity(top_ms->ms_matrix);
}

void
matrix_stack_translate(fix16_t x, fix16_t y, fix16_t z)
{
        /* Make sure the correct state is set */
        assert(_state.initialized);
        assert(_state.mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_state.mode);
        assert(top_ms != NULL);

        fix16_matrix3_t transform;
        fix16_matrix3_identity(&transform);

        transform.frow[0][2] = x;
        transform.frow[1][2] = y;
        transform.frow[2][2] = z;

        fix16_matrix3_t matrix;
        fix16_matrix3_multiply(top_ms->ms_matrix, &transform, &matrix);

        matrix_stack_load(&matrix);
}
