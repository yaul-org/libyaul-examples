#include "mic3d.h"

#define INDICES(a, b, c, d) { .p0 = a, .p1 = b, .p2 = c, .p3 = d }

static const fix16_vec3_t _points_m[29] = {
        FIX16_VEC3_INITIALIZER( 0,  0,  0),
        FIX16_VEC3_INITIALIZER( 5, -3,  2),
        FIX16_VEC3_INITIALIZER( 3, -3,  2),
        FIX16_VEC3_INITIALIZER( 3,  3,  2),
        FIX16_VEC3_INITIALIZER( 5,  3,  2),
        FIX16_VEC3_INITIALIZER( 5, -3, -2),
        FIX16_VEC3_INITIALIZER(-3, -3, -2),
        FIX16_VEC3_INITIALIZER(-3, -3,  2),
        FIX16_VEC3_INITIALIZER(-3, -1,  2),
        FIX16_VEC3_INITIALIZER( 3, -1,  2),
        FIX16_VEC3_INITIALIZER(-5, -1,  2),
        FIX16_VEC3_INITIALIZER(-5, -1, -2),
        FIX16_VEC3_INITIALIZER(-5,  3, -2),
        FIX16_VEC3_INITIALIZER(-5,  3,  2),
        FIX16_VEC3_INITIALIZER(-3,  3,  2),
        //
        FIX16_VEC3_INITIALIZER(-3,  3, -2),
        FIX16_VEC3_INITIALIZER(-1, -1,  2),
        FIX16_VEC3_INITIALIZER(-1, -1, -2),
        FIX16_VEC3_INITIALIZER(-1,  3, -2),
        FIX16_VEC3_INITIALIZER(-1,  3,  2),
        FIX16_VEC3_INITIALIZER( 1,  3,  2),
        FIX16_VEC3_INITIALIZER( 1,  3, -2),
        FIX16_VEC3_INITIALIZER( 1, -1,  2),
        FIX16_VEC3_INITIALIZER( 3, -1, -2),
        FIX16_VEC3_INITIALIZER( 3,  3, -2),
        FIX16_VEC3_INITIALIZER( 5,  3, -2),
        FIX16_VEC3_INITIALIZER( 3, -3, -2),
        FIX16_VEC3_INITIALIZER(-3, -1, -2),
        FIX16_VEC3_INITIALIZER( 1, -1, -2)
};

static const attribute_t _attributes_m[23] = {
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) }
};

static const polygon_t _polygons_m[23] = {
        INDICES(  8,  8, 10,  7),
        INDICES( 11, 11, 27,  6),
        INDICES(  5,  1,  7,  6),
        INDICES(  7,  2,  9,  8),
        INDICES(  6,  7, 10, 11),
        INDICES( 11, 10, 13, 12),
        INDICES( 13, 14, 15, 12),
        INDICES( 10,  8, 14, 13 ),
        INDICES( 17, 16, 19, 18),
        INDICES( 19, 20, 21, 18),
        INDICES( 16, 22, 20, 19),
        INDICES( 23,  9,  3, 24),
        INDICES(  3,  4, 25, 24),
        INDICES(  1,  5, 25,  4),
        INDICES( 25,  5, 26, 24),
        INDICES( 23, 26,  6, 27),
        INDICES(  8, 27, 15, 14),
        INDICES( 15, 27, 11, 12),
        INDICES( 16, 17, 27,  8),
        INDICES( 22, 28, 21, 20),
        INDICES( 21, 28, 17, 18),
        INDICES(  9, 23, 28, 22),
        INDICES(  2,  1,  4,  3)
};

const mesh_t mesh_m = {
        .points         = _points_m,
        .points_count   = 29,
        .polygons       = _polygons_m,
        .attributes     = _attributes_m,
        .polygons_count = 23
};
