/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdlib.h>

#include <yaul.h>

#include "scene.h"

#define ORDER_SYSTEM_CLIP_COORDS_INDEX      0
#define ORDER_CLEAR_LOCAL_COORDS_INDEX      1
#define ORDER_CLEAR_POLYGON_INDEX           2
#define ORDER_BUFFER_STARTING_INDEX         3
#define ORDER_BUFFER_END_INDEX              (ORDER_BUFFER_STARTING_INDEX + 256)
#define ORDER_DRAW_END_INDEX                (ORDER_BUFFER_END_INDEX + 1)
#define ORDER_COUNT                         ORDER_DRAW_END_INDEX

static constexpr uint32_t _screen_width = 320;
static constexpr uint32_t _screen_height = 240;

static constexpr uint32_t _render_width = 256;
static constexpr uint32_t _render_height = 200;

static constexpr fix16_t _scale_width = FIX16(_screen_width / static_cast<float>(_render_width));
static constexpr fix16_t _scale_height = FIX16(_screen_height / static_cast<float>(_render_height));

static const char* _scene_file_path = "SCENE.BIN";

static void* _romdisk;

static vdp1_cmdt_list_t* _scene_cmdt_list;
static uint32_t _cmdt_buffer_index;

static color_rgb1555_t _palette[16] __aligned(32);

static void _hardware_init(void);
static void _romdisk_init(void);

static void _draw_init(void);

static void _on_start(uint32_t, bool);
static void _on_end(uint32_t, bool);
static void _on_clear_screen(bool);
static void _on_update_palette(uint8_t, const scene::rgb444);
static void _on_draw(int16_vec2_t const *, size_t, uint8_t);

static void _scene_cmdt_polygon_add(const int16_vec2_t&,
                                    const int16_vec2_t&,
                                    const int16_vec2_t&,
                                    const int16_vec2_t&,
                                    const uint8_t);

void main(void) {
    _hardware_init();
    _romdisk_init();

    dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
    dbgio_dev_font_load();
    dbgio_dev_font_load_wait();

    cpu_cache_disable();

    _draw_init();

    void *fh = romdisk_open(_romdisk, _scene_file_path);
    assert(fh != NULL);
    void *scene_ptr = romdisk_direct(fh);
    const uint8_t* scene_buffer = static_cast<uint8_t*>(scene_ptr);

    scene::callbacks callbacks;
    callbacks.on_start = _on_start;
    callbacks.on_end = _on_end;
    callbacks.on_clear_screen = _on_clear_screen;
    callbacks.on_update_palette = _on_update_palette;
    callbacks.on_draw = _on_draw;

    scene::init(scene_buffer, callbacks);

    while (true) {
        scene::process_frame();

        vdp1_sync_cmdt_list_put(_scene_cmdt_list, NULL, NULL);
        vdp_sync();
    }

    __builtin_unreachable();
}

static void _hardware_init(void) {
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_240);

    vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE), COLOR_RGB1555(1, 0, 0, 0));

    vdp2_sprite_priority_set(0, 6);

    cpu_intc_mask_set(0);

    vdp2_tvmd_display_set();

    vdp1_env_t vdp1_env;

    vdp1_env.erase_color = COLOR_RGB1555(1, 0, 0, 0);
    vdp1_env.erase_points[0].x = 0;
    vdp1_env.erase_points[0].y = 0;
    vdp1_env.erase_points[1].x = _screen_width - 1;
    vdp1_env.erase_points[1].y = _screen_height - 1;
    vdp1_env.bpp = VDP1_ENV_BPP_16;
    vdp1_env.rotation = VDP1_ENV_ROTATION_0;
    vdp1_env.color_mode = VDP1_ENV_COLOR_MODE_RGB_PALETTE;
    vdp1_env.sprite_type = 0x0;

    vdp1_env_set(&vdp1_env);
}

static void _romdisk_init(void) {
    extern uint8_t root_romdisk[];

    romdisk_init();

    _romdisk = romdisk_mount("/", root_romdisk);
    assert(_romdisk != NULL);
}

static void _draw_init(void) {
    constexpr int16_vec2_t system_clip_coord =
        INT16_VEC2_INITIALIZER(_screen_width - 1,
                               _screen_height - 1);

    constexpr int16_vec2_t local_coord_ul =
        INT16_VEC2_INITIALIZER(0,
                               0);

    vdp1_cmdt_draw_mode_t polygon_draw_mode;
    polygon_draw_mode.raw = 0x0000;
    polygon_draw_mode.bits.pre_clipping_disable = true;

    static constexpr int16_vec2_t polygon_points[] = {
        INT16_VEC2_INITIALIZER(                0, _screen_height - 1),
        INT16_VEC2_INITIALIZER(_screen_width - 1, _screen_height - 1),
        INT16_VEC2_INITIALIZER(_screen_width - 1,                  0),
        INT16_VEC2_INITIALIZER(                0,                  0)
    };

    _scene_cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);

    vdp1_cmdt_t* const cmdts = _scene_cmdt_list->cmdts; 

    vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX], CMDT_VTX_SYSTEM_CLIP, &system_clip_coord);

    vdp1_cmdt_local_coord_set(&cmdts[ORDER_CLEAR_LOCAL_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_LOCAL_COORDS_INDEX], CMDT_VTX_LOCAL_COORD, &local_coord_ul);

    vdp1_cmdt_polygon_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX]);
    vdp1_cmdt_param_draw_mode_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], polygon_draw_mode);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], CMDT_VTX_POLYGON_A, &polygon_points[0]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], CMDT_VTX_POLYGON_B, &polygon_points[1]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], CMDT_VTX_POLYGON_C, &polygon_points[2]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], CMDT_VTX_POLYGON_D, &polygon_points[3]);
    vdp1_cmdt_jump_skip_next(&cmdts[ORDER_CLEAR_POLYGON_INDEX]);

    for (uint32_t i = ORDER_BUFFER_STARTING_INDEX; i < ORDER_BUFFER_END_INDEX; i++) {
        vdp1_cmdt_polygon_set(&cmdts[i]);
        vdp1_cmdt_param_draw_mode_set(&cmdts[i], polygon_draw_mode);
    }

    vdp1_cmdt_end_set(&cmdts[ORDER_BUFFER_STARTING_INDEX]);
}

static void _on_start(uint32_t, bool) {
    _cmdt_buffer_index = 0;
}

static void _on_end(uint32_t, bool last_frame) {
    _scene_cmdt_list->count = ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index + 1;

    if (last_frame) {
        scene::reset();
    }
}

static void _on_update_palette(uint8_t palette_index, const scene::rgb444 color) {
    const uint8_t scaled_r = color.r << 2;
    const uint8_t scaled_g = color.g << 2;
    const uint8_t scaled_b = color.b << 2;

    const color_rgb1555_t rgb1555_color = COLOR_RGB1555(1, scaled_r, scaled_g, scaled_b);

    _palette[palette_index] = rgb1555_color;

    (void)memcpy((void*)VDP2_CRAM_ADDR(0x10), &_palette[0], sizeof(_palette));
}

static void _on_clear_screen(bool clear_screen __unused) {
    // vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);

    if (!clear_screen) {
        vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);
    } else {
        vdp1_sync_mode_set(VDP1_SYNC_MODE_ERASE_CHANGE);
    }
}

static void _on_draw(int16_vec2_t const * vertex_buffer,
                     size_t count,
                     uint8_t palette_index) {
    switch (count) {
        case 3:
            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[1],
            //                         vertex_buffer[1],
            //                         vertex_buffer[2],
            //                         palette_index);
            break;
        case 4:
            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[1],
                                    vertex_buffer[2],
                                    vertex_buffer[3],
                                    palette_index);
            break;
        case 5:
            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[1],
            //                         vertex_buffer[1],
            //                         vertex_buffer[2],
            //                         palette_index);

            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[2],
            //                         vertex_buffer[2],
            //                         vertex_buffer[3],
            //                         palette_index);

            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[3],
            //                         vertex_buffer[3],
            //                         vertex_buffer[4],
            //                         palette_index);
            break;
        case 6:
            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[1],
            //                         vertex_buffer[1],
            //                         vertex_buffer[2],
            //                         palette_index);

            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[2],
            //                         vertex_buffer[2],
            //                         vertex_buffer[3],
            //                         palette_index);

            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[3],
            //                         vertex_buffer[3],
            //                         vertex_buffer[4],
            //                         palette_index);

            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[4],
            //                         vertex_buffer[4],
            //                         vertex_buffer[5],
            //                         palette_index);
            break;
        case 7:
            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[1],
            //                         vertex_buffer[1],
            //                         vertex_buffer[2],
            //                         palette_index);

            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[2],
            //                         vertex_buffer[2],
            //                         vertex_buffer[3],
            //                         palette_index);

            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[3],
            //                         vertex_buffer[3],
            //                         vertex_buffer[4],
            //                         palette_index);

            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[4],
            //                         vertex_buffer[4],
            //                         vertex_buffer[5],
            //                         palette_index);

            // _scene_cmdt_polygon_add(vertex_buffer[0],
            //                         vertex_buffer[5],
            //                         vertex_buffer[5],
            //                         vertex_buffer[6],
            //                         palette_index);
            break;
    }
}

static void _scene_cmdt_polygon_add(const int16_vec2_t& p0,
                                    const int16_vec2_t& p1,
                                    const int16_vec2_t& p2,
                                    const int16_vec2_t& p3,
                                    const uint8_t palette_index) {
    /* Specify the CRAM offset */
    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;
    color_bank.type_0.data.dc = palette_index + 0x10;

    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    vdp1_cmdt* const end_cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index + 1];

    vdp1_cmdt_polygon_set(cmdt);
    vdp1_cmdt_param_color_bank_set(cmdt, color_bank);

    cmdt->cmd_xa = fix16_int16_mul(_scale_width, p0.x) >> 16;
    cmdt->cmd_ya = fix16_int16_mul(_scale_height, p0.y) >> 16;

    cmdt->cmd_xb = fix16_int16_mul(_scale_width, p1.x) >> 16;
    cmdt->cmd_yb = fix16_int16_mul(_scale_height, p1.y) >> 16;

    cmdt->cmd_xc = fix16_int16_mul(_scale_width, p2.x) >> 16;
    cmdt->cmd_yc = fix16_int16_mul(_scale_height, p2.y) >> 16;

    cmdt->cmd_xd = fix16_int16_mul(_scale_width, p3.x) >> 16;
    cmdt->cmd_yd = fix16_int16_mul(_scale_height, p3.y) >> 16;

    vdp1_cmdt_end_set(end_cmdt);

    _cmdt_buffer_index++;
}
