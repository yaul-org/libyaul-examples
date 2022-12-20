#include "mic3d.h"

static point_t _points_c[22] = {
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

static polygon_t _polygons_c[16] = {
        { 0,  1,  3, 10},
        { 1,  2,  3,  3},
        {10,  4,  8,  9},
        { 5,  6,  7,  8},
        {11, 12, 14, 21},
        {12, 13, 14, 14},
        {21, 15, 19, 20},
        {16, 17, 18, 19},
        { 0, 11, 12,  1},
        { 1, 12, 13,  2},
        { 4, 15, 13,  2},
        { 4, 15, 16,  5},
        { 5, 16, 17,  6},
        { 6, 17, 18,  7},
        { 9, 20, 18,  7},
        {11,  0,  9, 20}
};

const mesh_t mesh_c = {
        .points         = _points_c,
        .points_count   = 22,
        .polygons       = _polygons_c,
        .polygons_count = 16
};
