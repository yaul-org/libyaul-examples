/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

SLIST_HEAD(matrix_stacks, matrix_stack);

MEMB(_matrix_stack_pool, struct matrix_stack, MATRIX_STACK_DEPTH,
    sizeof(struct matrix_stack));
MEMB(_matrix_stack_matrix_pool, fix16_matrix3_t, MATRIX_STACK_DEPTH,
    sizeof(fix16_matrix3_t));

static bool _initialized;
static int32_t _mode;
static struct matrix_stacks _matrix_stacks[MATRIX_STACK_MODES];

void
matrix_stack_init(void)
{
        if (_initialized) {
                return;
        }

        SLIST_INIT(&_matrix_stacks[MATRIX_STACK_MODE_PROJECTION]);
        SLIST_INIT(&_matrix_stacks[MATRIX_STACK_MODE_MODEL_VIEW]);

        memb_init(&_matrix_stack_pool);
        memb_init(&_matrix_stack_matrix_pool);

        _initialized = true;
        _mode = MATRIX_STACK_MODE_INVALID;

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
        assert(_initialized);
        assert((mode == MATRIX_STACK_MODE_PROJECTION) ||
               (mode == MATRIX_STACK_MODE_MODEL_VIEW));

        _mode = mode;
}

void
matrix_stack_push(void)
{
        /* Make sure the correct state is set */
        assert(_initialized);
        assert(_mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *ms;
        ms = (struct matrix_stack *)memb_alloc(&_matrix_stack_pool);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);

        ms->ms_matrix = (fix16_matrix3_t *)memb_alloc(
                &_matrix_stack_matrix_pool);
        assert(ms->ms_matrix != NULL);

        if (top_ms != NULL) {
                memcpy(ms->ms_matrix, top_ms->ms_matrix,
                    sizeof(fix16_matrix3_t));
        }

        SLIST_INSERT_HEAD(&_matrix_stacks[_mode], ms, ms_entries);
}

void
matrix_stack_pop(void)
{
        /* Make sure the correct state is set */
        assert(_initialized);
        assert(_mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);
        assert(top_ms != NULL);

        int error_code __unused;
        error_code = memb_free(&_matrix_stack_matrix_pool, top_ms->ms_matrix);
        assert(error_code == 0);
        error_code = memb_free(&_matrix_stack_pool, top_ms);
        assert(error_code == 0);
        SLIST_REMOVE_HEAD(&_matrix_stacks[_mode], ms_entries);

        /* Make sure we didn't pop off the last matrix in the stack */
        assert(!(SLIST_EMPTY(&_matrix_stacks[_mode])));
}

struct matrix_stack *
matrix_stack_top(int32_t mode)
{
        /* Make sure the correct state is set */
        assert(_initialized);
        assert((mode == MATRIX_STACK_MODE_PROJECTION) ||
               (mode == MATRIX_STACK_MODE_MODEL_VIEW));

        return SLIST_FIRST(&_matrix_stacks[mode]);
}

void
matrix_stack_load(fix16_matrix3_t *matrix)
{
        /* Make sure the correct state is set */
        assert(_initialized);
        assert(_mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);
        assert(top_ms != NULL);

        memcpy(top_ms->ms_matrix, matrix, sizeof(fix16_matrix3_t));
}

void
matrix_stack_identity_load(void)
{
        /* Make sure the correct state is set */
        assert(_initialized);
        assert(_mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);
        assert(top_ms != NULL);

        fix16_matrix3_identity(top_ms->ms_matrix);
}

void
matrix_stack_translate(fix16_t x, fix16_t y, fix16_t z)
{
        /* Make sure the correct state is set */
        assert(_initialized);
        assert(_mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);
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
