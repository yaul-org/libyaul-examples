#include "mic3d.h"

#define INDICES(a, b, c, d) {.p0 = a, .p1 = b, .p2 = c, .p3 = d}

static const fix16_vec3_t _points_i[10] = {
        FIX16_VEC3_INITIALIZER(-1, -3, -2),
        FIX16_VEC3_INITIALIZER( 1, -1, -2),
        FIX16_VEC3_INITIALIZER( 1,  3, -2),
        FIX16_VEC3_INITIALIZER(-1,  3, -2),
        FIX16_VEC3_INITIALIZER(-1, -1, -2),
        //
        FIX16_VEC3_INITIALIZER(-1, -3,  2),
        FIX16_VEC3_INITIALIZER( 1, -1,  2),
        FIX16_VEC3_INITIALIZER( 1,  3,  2),
        FIX16_VEC3_INITIALIZER(-1,  3,  2),
        FIX16_VEC3_INITIALIZER(-1, -1,  2)
};

static const attribute_t _attributes_i[] = {
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)},
        {.draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.plane_type = PLANE_TYPE_DOUBLE, .base_color = RGB1555(1, 15, 15, 15)}
};

static const polygon_t _polygons_i[8] = {
        INDICES(0, 1,  4, 4),
        INDICES(1, 2,  3, 4),
        INDICES(0, 5,  6, 1),
        INDICES(1, 6,  7, 2),
        INDICES(3, 8,  7, 2),
        INDICES(5, 0,  3, 8),
        INDICES(5, 6,  9, 9),
        INDICES(6, 7,  8, 9)
};

const mesh_t mesh_i = {
        .points         = _points_i,
        .points_count   = 10,
        .polygons       = _polygons_i,
        .attributes     = _attributes_i,
        .polygons_count = 8
};
