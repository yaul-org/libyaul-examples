#ifndef MIC3D_STATE_H
#define MIC3D_STATE_H

#include <fix16.h>
#include <mat_stack.h>

#include "mic3d.h"

#include "render.h"
#include "sort.h"
#include "tlist.h"
#include "matrix.h"
#include "camera.h"

typedef struct {
        render_t *render;
        sort_t *sort;
        tlist_t *tlist;
        mat_stack_t *mat_stack;
} state_t;

extern state_t __state;

#endif /* MIC3D_STATE_H */
