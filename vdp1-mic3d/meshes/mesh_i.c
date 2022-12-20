#include "mic3d.h"

static point_t _points_i[10] = {
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

static polygon_t _polygons_i[8] = {
        {0,  1,  4,  4},
        {1,  2,  3,  4},
        {0,  5,  6,  1},
        {1,  6,  7,  2},
        {3,  8,  7,  2},
        {5,  0,  3,  8},
        {5,  6,  9,  9},
        {6,  7,  8,  9}
};

const mesh_t mesh_i = {
        .points         = _points_i,
        .points_count   = 10,
        .polygons       = _polygons_i,
        .polygons_count = 8
};
