#include "mic3d.h"

#define VERTICES(a, b, c, d) {.p0 = a, .p1 = b, .p2 = c, .p3 = d}

static const point_t _points_c[22] = {
        {FIX16(-3), FIX16(-3), FIX16(-2)},
        {FIX16( 1), FIX16(-3), FIX16(-2)},
        {FIX16( 3), FIX16(-1), FIX16(-2)},
        {FIX16( 1), FIX16(-1), FIX16(-2)},
        {FIX16(-1), FIX16(-1), FIX16(-2)},
        {FIX16(-1), FIX16( 1), FIX16(-2)},
        {FIX16( 3), FIX16( 1), FIX16(-2)},
        {FIX16( 3), FIX16( 3), FIX16(-2)},
        {FIX16(-1), FIX16( 3), FIX16(-2)},
        {FIX16(-3), FIX16( 3), FIX16(-2)},
        {FIX16(-3), FIX16(-1), FIX16(-2)},
        //
        {FIX16(-3), FIX16(-3), FIX16(2)},
        {FIX16( 1), FIX16(-3), FIX16(2)},
        {FIX16( 3), FIX16(-1), FIX16(2)},
        {FIX16( 1), FIX16(-1), FIX16(2)},
        {FIX16(-1), FIX16(-1), FIX16(2)},
        {FIX16(-1), FIX16( 1), FIX16(2)},
        {FIX16( 3), FIX16( 1), FIX16(2)},
        {FIX16( 3), FIX16( 3), FIX16(2)},
        {FIX16(-1), FIX16( 3), FIX16(2)},
        {FIX16(-3), FIX16( 3), FIX16(2)},
        {FIX16(-3), FIX16(-1), FIX16(2)}
};

static const polygon_t _polygons_c[16] = {
        VERTICES( 0,  1,  3, 10),
        VERTICES( 1,  2,  3,  3),
        VERTICES(10,  4,  8,  9),
        VERTICES( 5,  6,  7,  8),
        VERTICES(11, 12, 14, 21),
        VERTICES(12, 13, 14, 14),
        VERTICES(21, 15, 19, 20),
        VERTICES(16, 17, 18, 19),
        VERTICES( 0, 11, 12,  1),
        VERTICES( 1, 12, 13,  2),
        VERTICES( 4, 15, 13,  2),
        VERTICES( 4, 15, 16,  5),
        VERTICES( 5, 16, 17,  6),
        VERTICES( 6, 17, 18,  7),
        VERTICES( 9, 20, 18,  7),
        VERTICES(11,  0,  9, 20)
};

const mesh_t mesh_c = {
        .points         = _points_c,
        .points_count   = 22,
        .polygons       = _polygons_c,
        .polygons_count = 16
};
