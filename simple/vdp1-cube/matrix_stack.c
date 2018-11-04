/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <string.h>

#include "matrix_stack.h"

SLIST_HEAD(matrix_stacks, matrix_stack);

MEMB(_matrix_stack_pool, struct matrix_stack, MATRIX_STACK_DEPTH,
    sizeof(struct matrix_stack));
MEMB(_matrix_stack_matrix_pool, fix16_matrix4_t, MATRIX_STACK_DEPTH,
    sizeof(fix16_matrix4_t));

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

        ms->ms_matrix = (fix16_matrix4_t *)memb_alloc(
                &_matrix_stack_matrix_pool);
        assert(ms->ms_matrix != NULL);

        if (top_ms != NULL) {
                memcpy(ms->ms_matrix, top_ms->ms_matrix,
                    sizeof(fix16_matrix4_t));
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
matrix_stack_load(fix16_matrix4_t *matrix)
{
        /* Make sure the correct state is set */
        assert(_initialized);
        assert(_mode != MATRIX_STACK_MODE_INVALID);

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);
        assert(top_ms != NULL);

        memcpy(top_ms->ms_matrix, matrix, sizeof(fix16_matrix4_t));
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

        fix16_matrix4_identity(top_ms->ms_matrix);
}

void
matrix_stack_translate(fix16_t x, fix16_t y, fix16_t z)
{
#ifdef ASSERT
        /* Make sure the correct state is set */
        assert(_initialized);
        assert(_mode != MATRIX_STACK_MODE_INVALID);
#endif /* ASSERT */

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);
#ifdef ASSERT
        assert(top_ms != NULL);
#endif /* ASSERT */

        fix16_matrix4_t transform;
        fix16_matrix4_identity(&transform);

        transform.frow[0][3] = x;
        transform.frow[1][3] = y;
        transform.frow[2][3] = z;

        fix16_matrix4_t matrix;
        fix16_matrix4_multiply(top_ms->ms_matrix, &transform, &matrix);

        matrix_stack_load(&matrix);
}

void
matrix_stack_scale(fix16_t x, fix16_t y, fix16_t z)
{
#ifdef ASSERT
        /* Make sure the correct state is set */
        assert(_initialized);
        assert(_mode != MATRIX_STACK_MODE_INVALID);
#endif /* ASSERT */

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);

#ifdef ASSERT
        assert(top_ms != NULL);
#endif /* ASSERT */

        fix16_matrix4_t transform;
        fix16_matrix4_identity(&transform);

        transform.frow[0][0] = x;
        transform.frow[1][1] = y;
        transform.frow[2][2] = z;

        fix16_matrix4_t matrix;
        fix16_matrix4_multiply(top_ms->ms_matrix, &transform, &matrix);

        matrix_stack_load(&matrix);
}

void
matrix_stack_rotate(fix16_t angle, int32_t component)
{
#ifdef ASSERT
        /* Make sure the correct state is set */
        assert(_initialized);
        assert(_mode != MATRIX_STACK_MODE_INVALID);

        assert((component >= 0) && (component <= 2));
#endif /* ASSERT */

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);
#ifdef ASSERT
        assert(top_ms != NULL);
#endif /* ASSERT */

        fix16_matrix4_t transform;
        fix16_matrix4_identity(&transform);

        fix16_t sin;
        sin = fix16_sin(fix16_deg_to_rad(angle));
        fix16_t cos;
        cos = fix16_cos(fix16_deg_to_rad(angle));

        switch (component) {
        case 0: /* X */
                transform.frow[1][1] = cos;
                transform.frow[1][2] = -sin;
                transform.frow[2][1] = sin;
                transform.frow[2][2] = cos;
                break;
        case 1: /* Y */
                transform.frow[0][0] = cos;
                transform.frow[0][2] = sin;
                transform.frow[2][0] = -sin;
                transform.frow[2][2] = cos;
                break;
        case 2: /* Z */
                transform.frow[0][0] = cos;
                transform.frow[0][1] = -sin;
                transform.frow[1][0] = sin;
                transform.frow[1][1] = cos;
                break;
        }

        fix16_matrix4_t matrix;
        fix16_matrix4_multiply(top_ms->ms_matrix, &transform, &matrix);

        matrix_stack_load(&matrix);
}

void
matrix_stack_orthographic_project(fix16_t left __unused, fix16_t right __unused, fix16_t top __unused,
    fix16_t bottom __unused, fix16_t near __unused, fix16_t far __unused)
{
#ifdef ASSERT
        /* Make sure the correct state is set */
        assert(state._initialized);
        assert(state._mode != MATRIX_STACK_MODE_INVALID);
#endif /* ASSERT */

        struct matrix_stack *top_ms;
        top_ms = matrix_stack_top(_mode);

#ifdef ASSERT
        assert(top_ms != NULL);
#endif /* ASSERT */

        fix16_matrix4_t transform;
        fix16_matrix4_identity(&transform);

#ifdef ASSERT
        assert(near > F16(0.0f));
#endif /* ASSERT */


        transform.frow[0][0] = fix16_div(F16(2.0f), fix16_sub(right, left));
        transform.frow[0][3] = fix16_div(-fix16_add(right, left), fix16_sub(right, left));
        transform.frow[1][1] = fix16_div(F16(2.0f), fix16_sub(top, bottom));
        transform.frow[1][3] = fix16_div(-fix16_add(top, bottom), fix16_sub(top, bottom));
        transform.frow[2][2] = fix16_div(F16(-2.0f), fix16_sub(far, near));
        transform.frow[2][3] = fix16_div(-fix16_add(far, near), fix16_sub(far, near));

        fix16_matrix4_t matrix;
        fix16_matrix4_multiply(top_ms->ms_matrix, &transform, &matrix);

        matrix_stack_load(&matrix);
}
