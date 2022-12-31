#include "mic3d.h"

#define VERTICES(a, b, c, d) {.p0 = a, .p1 = b, .p2 = c, .p3 = d}

static const point_t _points_i[10] = {
        {FIX16(-1), FIX16(-3), FIX16(-2)},
        {FIX16( 1), FIX16(-1), FIX16(-2)},
        {FIX16( 1), FIX16( 3), FIX16(-2)},
        {FIX16(-1), FIX16( 3), FIX16(-2)},
        {FIX16(-1), FIX16(-1), FIX16(-2)},
        //
        {FIX16(-1), FIX16(-3), FIX16( 2)},
        {FIX16( 1), FIX16(-1), FIX16( 2)},
        {FIX16( 1), FIX16( 3), FIX16( 2)},
        {FIX16(-1), FIX16( 3), FIX16( 2)},
        {FIX16(-1), FIX16(-1), FIX16( 2)}
};

static const polygon_t _polygons_i[8] = {
        VERTICES(0,  1,  4, 4),
        VERTICES(1,  2,  3, 4),
        VERTICES(0,  5,  6, 1),
        VERTICES(1,  6,  7, 2),
        VERTICES(3,  8,  7, 2),
        VERTICES(5,  0,  3, 8),
        VERTICES(5,  6,  9, 9),
        VERTICES(6,  7,  8, 9)
};

const mesh_t mesh_i = {
        .points         = _points_i,
        .points_count   = 10,
        .polygons       = _polygons_i,
        .polygons_count = 8
};
