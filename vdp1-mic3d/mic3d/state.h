#ifndef MIC3D_STATE_H
#define MIC3D_STATE_H

#include <fix16.h>

#include "mic3d.h"

#include "render.h"
#include "sort.h"
#include "tlist.h"

typedef struct {
        render_t *render;
        sort_t *sort;
        tlist_t *tlist;
} state_t;

extern state_t __state;

#endif /* MIC3D_STATE_H */
