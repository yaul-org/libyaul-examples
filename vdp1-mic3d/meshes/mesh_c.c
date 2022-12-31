#include "mic3d.h"

#define INDICES(a, b, c, d) {.p0 = a, .p1 = b, .p2 = c, .p3 = d}

static const fix16_vec3_t _points_c[22] = {
        FIX16_VEC3_INITIALIZER(-3, -3, -2),
        FIX16_VEC3_INITIALIZER( 1, -3, -2),
        FIX16_VEC3_INITIALIZER( 3, -1, -2),
        FIX16_VEC3_INITIALIZER( 1, -1, -2),
        FIX16_VEC3_INITIALIZER(-1, -1, -2),
        FIX16_VEC3_INITIALIZER(-1,  1, -2),
        FIX16_VEC3_INITIALIZER( 3,  1, -2),
        FIX16_VEC3_INITIALIZER( 3,  3, -2),
        FIX16_VEC3_INITIALIZER(-1,  3, -2),
        FIX16_VEC3_INITIALIZER(-3,  3, -2),
        FIX16_VEC3_INITIALIZER(-3, -1, -2),
        //
        FIX16_VEC3_INITIALIZER(-3, -3,  2),
        FIX16_VEC3_INITIALIZER( 1, -3,  2),
        FIX16_VEC3_INITIALIZER( 3, -1,  2),
        FIX16_VEC3_INITIALIZER( 1, -1,  2),
        FIX16_VEC3_INITIALIZER(-1, -1,  2),
        FIX16_VEC3_INITIALIZER(-1,  1,  2),
        FIX16_VEC3_INITIALIZER( 3,  1,  2),
        FIX16_VEC3_INITIALIZER( 3,  3,  2),
        FIX16_VEC3_INITIALIZER(-1,  3,  2),
        FIX16_VEC3_INITIALIZER(-3,  3,  2),
        FIX16_VEC3_INITIALIZER(-3, -1,  2)
};

static const attribute_t _attributes_c[] = {
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)}
};

static const polygon_t _polygons_c[16] = {
        INDICES( 0,  1,  3, 10),
        INDICES( 1,  2,  3,  3),
        INDICES(10,  4,  8,  9),
        INDICES( 5,  6,  7,  8),
        INDICES(11, 12, 14, 21),
        INDICES(12, 13, 14, 14),
        INDICES(21, 15, 19, 20),
        INDICES(16, 17, 18, 19),
        INDICES( 0, 11, 12,  1),
        INDICES( 1, 12, 13,  2),
        INDICES( 4, 15, 13,  2),
        INDICES( 4, 15, 16,  5),
        INDICES( 5, 16, 17,  6),
        INDICES( 6, 17, 18,  7),
        INDICES( 9, 20, 18,  7),
        INDICES(11,  0,  9, 20)
};

const mesh_t mesh_c = {
        .points         = _points_c,
        .points_count   = 22,
        .polygons       = _polygons_c,
        .attributes     = _attributes_c,
        .polygons_count = 16
};
