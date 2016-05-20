/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_MATRIX_STACK_H
#define ENGINE_MATRIX_STACK_H

#include "engine.h"

#define MATRIX_STACK_MODEL_VIEW_DEPTH   3
#define MATRIX_STACK_PROJECTION_DEPTH   1
#define MATRIX_STACK_DEPTH              (MATRIX_STACK_MODEL_VIEW_DEPTH +       \
    MATRIX_STACK_PROJECTION_DEPTH)

#define MATRIX_STACK_MODE_INVALID       -1
#define MATRIX_STACK_MODE_PROJECTION    0
#define MATRIX_STACK_MODE_MODEL_VIEW    1

#define MATRIX_STACK_MODES              2

struct matrix_stack {
        fix16_matrix3_t *ms_matrix;

        SLIST_ENTRY(matrix_stack) ms_entries;
} __aligned(4);

extern void matrix_stack_init(void);
extern void matrix_stack_mode(int32_t);
extern void matrix_stack_push(void);
extern void matrix_stack_pop(void);
extern struct matrix_stack *matrix_stack_top(int32_t);
extern void matrix_stack_load(fix16_matrix3_t *);

extern void matrix_stack_identity_load(void);
extern void matrix_stack_translate(fix16_t, fix16_t);

#endif /* !ENGINE_MATRIX_STACK_H */
