#include "mic3d.h"

#define INDICES(a, b, c, d) {.p0 = a, .p1 = b, .p2 = c, .p3 = d}

static const fix16_vec3_t _points_m[28] = {
        FIX16_VEC3_INITIALIZER(-5, -3, -2),
        FIX16_VEC3_INITIALIZER(-3, -3, -2),
        FIX16_VEC3_INITIALIZER( 3, -3, -2),
        FIX16_VEC3_INITIALIZER( 5, -1, -2),
        FIX16_VEC3_INITIALIZER( 5,  3, -2),
        FIX16_VEC3_INITIALIZER( 3,  3, -2),
        FIX16_VEC3_INITIALIZER( 3, -1, -2),
        FIX16_VEC3_INITIALIZER( 1, -1, -2),
        FIX16_VEC3_INITIALIZER( 1,  3, -2),
        FIX16_VEC3_INITIALIZER(-1,  3, -2),
        FIX16_VEC3_INITIALIZER(-1, -1, -2),
        FIX16_VEC3_INITIALIZER(-3, -1, -2),
        FIX16_VEC3_INITIALIZER(-3,  3, -2),
        FIX16_VEC3_INITIALIZER(-5,  3, -2),

        FIX16_VEC3_INITIALIZER(-5, -3,  2),
        FIX16_VEC3_INITIALIZER(-3, -3,  2),
        FIX16_VEC3_INITIALIZER( 3, -3,  2),
        FIX16_VEC3_INITIALIZER( 5, -1,  2),
        FIX16_VEC3_INITIALIZER( 5,  3,  2),
        FIX16_VEC3_INITIALIZER( 3,  3,  2),
        FIX16_VEC3_INITIALIZER( 3, -1,  2),
        FIX16_VEC3_INITIALIZER( 1, -1,  2),
        FIX16_VEC3_INITIALIZER( 1,  3,  2),
        FIX16_VEC3_INITIALIZER(-1,  3,  2),
        FIX16_VEC3_INITIALIZER(-1, -1,  2),
        FIX16_VEC3_INITIALIZER(-3, -1,  2),
        FIX16_VEC3_INITIALIZER(-3,  3,  2),
        FIX16_VEC3_INITIALIZER(-5,  3,  2)
};

static const attribute_t _attributes_m[] = {
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .base_color = RGB1555(1, 15, 15, 15)}
};

static const polygon_t _polygons_m[23] = {
        INDICES( 0,  1, 12, 13),
        INDICES( 0, 14, 16,  2),
        INDICES( 1,  2,  6, 11),
        INDICES( 2,  3,  6,  6),
        INDICES( 2, 16, 17,  3),
        INDICES( 3, 17, 18,  4),
        INDICES( 5, 19, 18,  4),
        INDICES( 6,  3,  4,  5),
        INDICES( 7, 21, 22,  8),
        INDICES( 9, 23, 22,  8),
        INDICES(10,  7,  8,  9),
        INDICES(11, 25, 26, 12),
        INDICES(13, 27, 26, 12),
        INDICES(14,  0, 13, 27),
        INDICES(14, 15, 26, 27),
        INDICES(15, 16, 20, 25),
        INDICES(16, 17, 20, 20),
        INDICES(20,  6,  5, 19),
        INDICES(20, 17, 18, 19),
        INDICES(21, 20,  6,  7),
        INDICES(24, 10,  9, 23),
        INDICES(24, 21, 22, 23),
        INDICES(25, 24, 10, 11)
};

const mesh_t mesh_m = {
        .points         = _points_m,
        .points_count   = 28,
        .polygons       = _polygons_m,
        .attributes     = _attributes_m,
        .polygons_count = 23
};
