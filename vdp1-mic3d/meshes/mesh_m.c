#include "mic3d.h"

#define VERTICES(a, b, c, d) {.p0 = a, .p1 = b, .p2 = c, .p3 = d}

static const point_t _points_m[28] = {
        {FIX16(-5), FIX16(-3), FIX16(-2)},
        {FIX16(-3), FIX16(-3), FIX16(-2)},
        {FIX16( 3), FIX16(-3), FIX16(-2)},
        {FIX16( 5), FIX16(-1), FIX16(-2)},
        {FIX16( 5), FIX16( 3), FIX16(-2)},
        {FIX16( 3), FIX16( 3), FIX16(-2)},
        {FIX16( 3), FIX16(-1), FIX16(-2)},
        {FIX16( 1), FIX16(-1), FIX16(-2)},
        {FIX16( 1), FIX16( 3), FIX16(-2)},
        {FIX16(-1), FIX16( 3), FIX16(-2)},
        {FIX16(-1), FIX16(-1), FIX16(-2)},
        {FIX16(-3), FIX16(-1), FIX16(-2)},
        {FIX16(-3), FIX16( 3), FIX16(-2)},
        {FIX16(-5), FIX16( 3), FIX16(-2)},

        {FIX16(-5), FIX16(-3), FIX16( 2)},
        {FIX16(-3), FIX16(-3), FIX16( 2)},
        {FIX16( 3), FIX16(-3), FIX16( 2)},
        {FIX16( 5), FIX16(-1), FIX16( 2)},
        {FIX16( 5), FIX16( 3), FIX16( 2)},
        {FIX16( 3), FIX16( 3), FIX16( 2)},
        {FIX16( 3), FIX16(-1), FIX16( 2)},
        {FIX16( 1), FIX16(-1), FIX16( 2)},
        {FIX16( 1), FIX16( 3), FIX16( 2)},
        {FIX16(-1), FIX16( 3), FIX16( 2)},
        {FIX16(-1), FIX16(-1), FIX16( 2)},
        {FIX16(-3), FIX16(-1), FIX16( 2)},
        {FIX16(-3), FIX16( 3), FIX16( 2)},
        {FIX16(-5), FIX16( 3), FIX16( 2)}
};

static const polygon_t _polygons_m[23] = {
        VERTICES( 0,  1, 12, 13),
        VERTICES( 0, 14, 16,  2),
        VERTICES( 1,  2,  6, 11),
        VERTICES( 2,  3,  6,  6),
        VERTICES( 2, 16, 17,  3),
        VERTICES( 3, 17, 18,  4),
        VERTICES( 5, 19, 18,  4),
        VERTICES( 6,  3,  4,  5),
        VERTICES( 7, 21, 22,  8),
        VERTICES( 9, 23, 22,  8),
        VERTICES(10,  7,  8,  9),
        VERTICES(11, 25, 26, 12),
        VERTICES(13, 27, 26, 12),
        VERTICES(14,  0, 13, 27),
        VERTICES(14, 15, 26, 27),
        VERTICES(15, 16, 20, 25),
        VERTICES(16, 17, 20, 20),
        VERTICES(20,  6,  5, 19),
        VERTICES(20, 17, 18, 19),
        VERTICES(21, 20,  6,  7),
        VERTICES(24, 10,  9, 23),
        VERTICES(24, 21, 22, 23),
        VERTICES(25, 24, 10, 11)
};

const mesh_t mesh_m = {
        .points         = _points_m,
        .points_count   = 28,
        .polygons       = _polygons_m,
        .polygons_count = 23
};
