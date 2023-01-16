#include <mic3d.h>

#define INDICES(a, b, c, d) { .indices.p0 = a, .indices.p1 = b, .indices.p2 = c, .indices.p3 = d }

static const fix16_vec3_t _points_i[9] = {
        FIX16_VEC3_INITIALIZER( 0,  0,  0),
        FIX16_VEC3_INITIALIZER( 1, -3,  2),
        FIX16_VEC3_INITIALIZER(-1, -1,  2),
        FIX16_VEC3_INITIALIZER(-1,  3,  2),
        FIX16_VEC3_INITIALIZER( 1,  3,  2),
        FIX16_VEC3_INITIALIZER( 1, -3, -2),
        FIX16_VEC3_INITIALIZER(-1, -1, -2),
        FIX16_VEC3_INITIALIZER(-1,  3, -2),
        FIX16_VEC3_INITIALIZER( 1,  3, -2)
};

static const attribute_t _attributes_i[6] = {
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) },
        { .draw_mode.raw = 0, .control.command = COMMAND_TYPE_POLYGON, .control.sort_type = SORT_TYPE_CENTER, .palette.base_color = RGB1555(1, 15, 15, 15) }
};

static const polygon_t _polygons_i[6] = {
        INDICES(4, 8, 7, 3),
        INDICES(2, 6, 5, 1),
        INDICES(3, 7, 6, 2),
        INDICES(5, 6, 7, 8),
        INDICES(3, 2, 1, 4),
        INDICES(5, 8, 4, 1)
};

const mesh_t mesh_i = {
        .points         = _points_i,
        .points_count   = 9,
        .polygons       = _polygons_i,
        .attributes     = _attributes_i,
        .polygons_count = 6
};
