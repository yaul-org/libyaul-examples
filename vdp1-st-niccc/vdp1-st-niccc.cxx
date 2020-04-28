/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdlib.h>

#include <yaul.h>

#include "scene.h"

static constexpr uint32_t _screen_width = 320;
static constexpr uint32_t _screen_height = 240;

static constexpr uint32_t _render_width = 256;
static constexpr uint32_t _render_height = 200;

static constexpr fix16_t _scale_width = F16(_screen_width / static_cast<float>(_render_width));
static constexpr fix16_t _scale_height = F16(_screen_height / static_cast<float>(_render_height));

static const char* _scene_file_path = "SCENE.BIN";

static void* _romdisk;

static vdp1_cmdt_list_t* _env_cmdt_list;
static vdp1_cmdt_list_t* _scene_cmdt_list;
static uint32_t _clear_polygon_index;

static color_rgb1555_t _palette[16] __aligned(32);

static void _hardware_init(void);
static void _romdisk_init(void);

static void _draw_init(void);

static void _on_start(uint32_t, bool);
static void _on_end(uint32_t, bool);
static void _on_clear_screen(bool);
static void _on_update_palette(uint8_t, const scene::rgb444);
static void _on_draw(int16_vector2_t const *, size_t, uint8_t);

static void _scene_cmdt_polygon_add(const int16_vector2_t&,
                                    const int16_vector2_t&,
                                    const int16_vector2_t&,
                                    const int16_vector2_t&,
                                    const uint8_t);

void main(void) {
    _hardware_init();
    _romdisk_init();

    dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
    dbgio_dev_font_load();
    dbgio_dev_font_load_wait();

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

        vdp1_sync_draw(_scene_cmdt_list, NULL, NULL);
        vdp_sync();
    }

    __builtin_unreachable();
}

static void _hardware_init(void) {
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_240);

    vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE), COLOR_RGB1555(0, 0, 0));

    vdp2_sprite_priority_set(0, 6);

    cpu_intc_mask_set(0);

    vdp2_tvmd_display_set();

    smpc_init();

    vdp1_env_t vdp1_env;

    vdp1_env.erase_color = COLOR_RGB1555(0, 0, 0);
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
    _scene_cmdt_list = vdp1_cmdt_list_alloc(256);
    vdp1_cmdt_end(_scene_cmdt_list);

    _env_cmdt_list = vdp1_cmdt_list_alloc(4);

    vdp1_cmdt_t_system_clip_coord cmdt_system_clip;
    cmdt_system_clip.coord.x = _screen_width - 1;
    cmdt_system_clip.coord.y = _screen_height - 1;

    vdp1_cmdt_t_local_coord cmdt_local_coord;
    cmdt_local_coord.coord.x = 0;
    cmdt_local_coord.coord.y = 0;

    vdp1_cmdt_t_polygon cmdt_polygon;
    cmdt_polygon.draw_mode.raw = 0x0000;
    cmdt_polygon.draw_mode.pre_clipping_disable = true;
    cmdt_polygon.color.raw = 0x0000;
    cmdt_polygon.vertices[0].x = 0;
    cmdt_polygon.vertices[0].y = _screen_height - 1;

    cmdt_polygon.vertices[1].x = _screen_width - 1;
    cmdt_polygon.vertices[1].y = _screen_height - 1;

    cmdt_polygon.vertices[2].x = _screen_width - 1;
    cmdt_polygon.vertices[2].y = 0;

    cmdt_polygon.vertices[3].x = 0;
    cmdt_polygon.vertices[3].y = 0;

    vdp1_cmdt_system_clip_coord_add(_env_cmdt_list, &cmdt_system_clip);
    vdp1_cmdt_local_coord_add(_env_cmdt_list, &cmdt_local_coord);
    _clear_polygon_index = vdp1_cmdt_polygon_add(_env_cmdt_list, &cmdt_polygon);
    vdp1_cmdt_end(_env_cmdt_list);
}

static void _on_start(uint32_t, bool) {
    vdp1_cmdt_list_reset(_scene_cmdt_list);
}

static void _on_end(uint32_t, bool last_frame) {
    vdp1_cmdt_end(_scene_cmdt_list);

    if (last_frame) {
        scene::reset();
    }
}

static void _on_update_palette(uint8_t palette_index, const scene::rgb444 color) {
    uint8_t scaled_r = color.r << 2;
    uint8_t scaled_g = color.g << 2;
    uint8_t scaled_b = color.b << 2;

    color_rgb1555_t rgb1555_color = COLOR_RGB1555(scaled_r, scaled_g, scaled_b);

    _palette[palette_index] = rgb1555_color;

    (void)memcpy((void*)VDP2_CRAM_ADDR(0x10), &_palette[0], sizeof(_palette));
}

static void _on_clear_screen(bool clear_screen) {
    vdp1_cmdt_jump_clear(_env_cmdt_list, _clear_polygon_index);

    if (!clear_screen) {
        vdp1_cmdt_jump_skip_next(_env_cmdt_list, _clear_polygon_index);
    }

    vdp1_sync_draw(_env_cmdt_list, NULL, NULL);
}

static void _on_draw(int16_vector2_t const * vertex_buffer, size_t count, uint8_t palette_index) {
    switch (count) {
        case 3:
            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[1],
                                    vertex_buffer[1],
                                    vertex_buffer[2],
                                    palette_index);
            break;
        case 4:
            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[1],
                                    vertex_buffer[2],
                                    vertex_buffer[3],
                                    palette_index);
            break;
        case 5:
            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[1],
                                    vertex_buffer[1],
                                    vertex_buffer[2],
                                    palette_index);

            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[2],
                                    vertex_buffer[2],
                                    vertex_buffer[3],
                                    palette_index);

            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[3],
                                    vertex_buffer[3],
                                    vertex_buffer[4],
                                    palette_index);
            break;
        case 6:
            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[1],
                                    vertex_buffer[1],
                                    vertex_buffer[2],
                                    palette_index);

            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[2],
                                    vertex_buffer[2],
                                    vertex_buffer[3],
                                    palette_index);

            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[3],
                                    vertex_buffer[3],
                                    vertex_buffer[4],
                                    palette_index);

            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[4],
                                    vertex_buffer[4],
                                    vertex_buffer[5],
                                    palette_index);
            break;
        case 7:
            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[1],
                                    vertex_buffer[1],
                                    vertex_buffer[2],
                                    palette_index);

            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[2],
                                    vertex_buffer[2],
                                    vertex_buffer[3],
                                    palette_index);

            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[3],
                                    vertex_buffer[3],
                                    vertex_buffer[4],
                                    palette_index);

            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[4],
                                    vertex_buffer[4],
                                    vertex_buffer[5],
                                    palette_index);

            _scene_cmdt_polygon_add(vertex_buffer[0],
                                    vertex_buffer[5],
                                    vertex_buffer[5],
                                    vertex_buffer[6],
                                    palette_index);
            break;
    }
}

static void _scene_cmdt_polygon_add(const int16_vector2_t& p0,
                                    const int16_vector2_t& p1,
                                    const int16_vector2_t& p2,
                                    const int16_vector2_t& p3,
                                    uint8_t palette_index) {
    vdp1_cmdt_t_polygon polygon;
    polygon.draw_mode.raw = 0x0000;
    polygon.draw_mode.pre_clipping_disable = true;

    /* Specify the CRAM offset */
    polygon.sprite_type.raw = 0x0000;
    polygon.sprite_type.type_0.data.dc = palette_index + 0x10;

    polygon.vertices[0].x = (p0.x * _scale_width) >> 16;
    polygon.vertices[0].y = (p0.y * _scale_height) >> 16;

    polygon.vertices[1].x = (p1.x * _scale_width) >> 16;
    polygon.vertices[1].y = (p1.y * _scale_height) >> 16;

    polygon.vertices[2].x = (p2.x * _scale_width) >> 16;
    polygon.vertices[2].y = (p2.y * _scale_height) >> 16;

    polygon.vertices[3].x = (p3.x * _scale_width) >> 16;
    polygon.vertices[3].y = (p3.y * _scale_height) >> 16;

    vdp1_cmdt_polygon_add(_scene_cmdt_list, &polygon);
}
