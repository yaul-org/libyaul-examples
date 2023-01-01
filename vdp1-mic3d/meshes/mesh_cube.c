#include "mic3d.h"

#define INDICES(a, b, c, d) {.p0 = a, .p1 = b, .p2 = c, .p3 = d}

static const fix16_vec3_t point_cube1[] = {
        FIX16_VEC3_INITIALIZER(-10.0, -10.0,  10.0), /* 0 */
        FIX16_VEC3_INITIALIZER( 10.0, -10.0,  10.0), /* 1 */
        FIX16_VEC3_INITIALIZER( 10.0,  10.0,  10.0), /* 2 */
        FIX16_VEC3_INITIALIZER(-10.0,  10.0,  10.0), /* 3 */
        FIX16_VEC3_INITIALIZER(-10.0, -10.0, -10.0), /* 4 */
        FIX16_VEC3_INITIALIZER( 10.0, -10.0, -10.0), /* 5 */
        FIX16_VEC3_INITIALIZER( 10.0,  10.0, -10.0), /* 6 */
        FIX16_VEC3_INITIALIZER(-10.0,  10.0, -10.0)  /* 7 */
};

static const attribute_t polygon_attributes[] = {
        { .draw_mode.bits.color_mode = 5, .control.use_texture = true, .control.command = COMMAND_TYPE_DISTORTED_SPRITE, .control.sort_type = SORT_TYPE_CENTER, .texture_slot = 0, .palette.base_color = RGB1555(1, 31,  0,  0) },
        { .draw_mode.bits.color_mode = 5, .control.use_texture = true, .control.command = COMMAND_TYPE_DISTORTED_SPRITE, .control.sort_type = SORT_TYPE_CENTER, .texture_slot = 1, .palette.base_color = RGB1555(1,  0, 31,  0) },
        { .draw_mode.bits.color_mode = 4, .control.use_texture = true, .control.command = COMMAND_TYPE_DISTORTED_SPRITE, .control.sort_type = SORT_TYPE_CENTER, .texture_slot = 2, .palette.color_bank.type_0.data.dc = 0x0000 },
        { .draw_mode.bits.color_mode = 5, .control.use_texture = true, .control.command = COMMAND_TYPE_DISTORTED_SPRITE, .control.sort_type = SORT_TYPE_CENTER, .texture_slot = 0, .palette.base_color = RGB1555(1, 31, 31,  0) },
        { .draw_mode.bits.color_mode = 5, .control.use_texture = true, .control.command = COMMAND_TYPE_DISTORTED_SPRITE, .control.sort_type = SORT_TYPE_CENTER, .texture_slot = 1, .palette.base_color = RGB1555(1,  0, 31, 31) },
        { .draw_mode.bits.color_mode = 4, .control.use_texture = true, .control.command = COMMAND_TYPE_DISTORTED_SPRITE, .control.sort_type = SORT_TYPE_CENTER, .texture_slot = 2, .palette.color_bank.type_0.data.dc = 0x0000 }
};

static const polygon_t polygon_cube1[] = {
        INDICES(0, 1, 2, 3), /* Back */
        INDICES(4, 0, 3, 7), /* Left */
        INDICES(5, 4, 7, 6), /* Front */
        INDICES(1, 5, 6, 2), /* Right */
        INDICES(4, 5, 1, 0), /* Top */
        INDICES(3, 2, 6, 7)  /* Bottom */
};

const mesh_t mesh_cube = {
        .points         = point_cube1,
        .points_count   = 8,
        .polygons       = polygon_cube1,
        .attributes     = polygon_attributes,
        .polygons_count = 6
};
