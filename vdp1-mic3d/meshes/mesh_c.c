#include "mic3d.h"

#define INDICES(a, b, c, d) { .indices.p0 = a, .indices.p1 = b, .indices.p2 = c, .indices.p3 = d }

static const fix16_vec3_t _points_c[27] = {
        FIX16_VEC3_INITIALIZER( 0,  0,  0),
        FIX16_VEC3_INITIALIZER( 3, -3,  2),
        FIX16_VEC3_INITIALIZER(-1, -3,  2),
        FIX16_VEC3_INITIALIZER(-1, -1,  2),
        FIX16_VEC3_INITIALIZER( 3, -1,  2),
        FIX16_VEC3_INITIALIZER(-3, -1,  2),
        FIX16_VEC3_INITIALIZER( 1, -1,  2),
        FIX16_VEC3_INITIALIZER( 1,  3,  2),
        FIX16_VEC3_INITIALIZER( 3,  3,  2),
        FIX16_VEC3_INITIALIZER( 1,  1,  2),
        FIX16_VEC3_INITIALIZER(-3,  1,  2),
        FIX16_VEC3_INITIALIZER(-3,  3,  2),
        FIX16_VEC3_INITIALIZER( 3, -3, -2),
        //
        FIX16_VEC3_INITIALIZER(-1, -3, -2),
        FIX16_VEC3_INITIALIZER(-1, -1, -2),
        FIX16_VEC3_INITIALIZER( 3, -1, -2),
        FIX16_VEC3_INITIALIZER(-3, -1, -2),
        FIX16_VEC3_INITIALIZER( 1, -1, -2),
        FIX16_VEC3_INITIALIZER( 1,  3, -2),
        FIX16_VEC3_INITIALIZER( 3,  3, -2),
        FIX16_VEC3_INITIALIZER( 1,  1, -2),
        FIX16_VEC3_INITIALIZER(-3,  1, -2),
        FIX16_VEC3_INITIALIZER(-3,  3, -2),
        FIX16_VEC3_INITIALIZER( 1, -3, -2),
        FIX16_VEC3_INITIALIZER( 1, -3,  2),
        FIX16_VEC3_INITIALIZER( 3,  1,  2),
        FIX16_VEC3_INITIALIZER( 3,  1, -2)
};

static const attribute_t _attributes_c[25] = {
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
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) }
};

static const polygon_t _polygons_c[25] = {
        INDICES( 8, 25, 26, 19),
        INDICES( 4,  1, 12, 15),
        INDICES(25,  4, 15, 26),
        INDICES(24,  6,  3,  2),
        INDICES( 4,  6, 24,  1),
        INDICES( 2, 13, 23, 24),
        INDICES( 1, 24, 23, 12),
        INDICES(17, 14,  3,  6),
        INDICES(16,  5,  3, 14),
        INDICES( 3,  3,  5,  2),
        INDICES(13,  2,  5, 16),
        INDICES(14, 14, 13, 16),
        INDICES(23, 13, 14, 17),
        INDICES(12, 23, 17, 15),
        INDICES(17,  6,  9, 20),
        INDICES(10, 21, 20,  9),
        INDICES(11, 22, 21, 10),
        INDICES( 7, 11, 10,  9),
        INDICES( 4, 25,  9,  6),
        INDICES(25,  8,  7,  9),
        INDICES( 8, 19, 18,  7),
        INDICES(11,  7, 18, 22),
        INDICES(26, 20, 18, 19),
        INDICES(18, 20, 21, 22),
        INDICES(15, 17, 20, 26)
};

const mesh_t mesh_c = {
        .points         = _points_c,
        .points_count   = 27,
        .polygons       = _polygons_c,
        .attributes     = _attributes_c,
        .polygons_count = 25
};
