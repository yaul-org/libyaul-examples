/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdlib.h>

#include <yaul.h>

#include "scene.h"

// #pragma GCC push_options
// #pragma GCC optimize ("align-functions=16")

#define ORDER_SYSTEM_CLIP_COORDS_INDEX      0
#define ORDER_LOCAL_COORDS_INDEX            1
#define ORDER_ERASE_INDEX                   2
#define ORDER_BUFFER_STARTING_INDEX         3
#define ORDER_BUFFER_END_INDEX              (ORDER_BUFFER_STARTING_INDEX + 512)
#define ORDER_DRAW_END_INDEX                (ORDER_BUFFER_END_INDEX + 1)
#define ORDER_COUNT                         ORDER_DRAW_END_INDEX

typedef uint32_t (*draw_handler)(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, color_rgb1555_t color);

static constexpr uint32_t _screen_width = 352;
static constexpr uint32_t _screen_height = 240;

static constexpr uint32_t _render_width = 256;
static constexpr uint32_t _render_height = 200;

static constexpr fix16_t _scale_width = FIX16(_screen_width / static_cast<float>(_render_width));
static constexpr fix16_t _scale_height = FIX16(_screen_height / static_cast<float>(_render_height));

static constexpr char* _scene_file_path = "SCENE.BIN";

static void* _romdisk;

static smpc_peripheral_digital_t _digital;

static vdp1_cmdt_list_t* _scene_cmdt_list;
static uint32_t _cmdt_buffer_index;

static volatile struct {
    uint32_t frame_count;
    uint32_t vblank_count;
    uint32_t dropped_count;
} _stats = {
    .frame_count = 0,
    .vblank_count = 0,
    .dropped_count = 0
};

static color_rgb1555_t _palette[16] __aligned(16);

static void _romdisk_init(void);

static void _draw_init(void);

static void _vblank_out_handler(void *);

static void _on_start(uint32_t, bool);
static void _on_end(uint32_t, bool);
static void _on_clear_screen(bool);
static void _on_update_palette(uint8_t, const scene::rgb444);
static void _on_draw(const uint8_vec2_t *, uint32_t, uint32_t);

static uint32_t _on_draw_polygon3(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, color_rgb1555_t color);
static uint32_t _on_draw_polygon4(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, color_rgb1555_t color);
static uint32_t _on_draw_polygon5(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, color_rgb1555_t color);
static uint32_t _on_draw_polygon6(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, color_rgb1555_t color);
static uint32_t _on_draw_polygon7(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, color_rgb1555_t color);

static const draw_handler _draw_handlers[] = {
    nullptr, // 0
    nullptr, // 1
    nullptr, // 2
    _on_draw_polygon3,
    _on_draw_polygon4,
    _on_draw_polygon5,
    _on_draw_polygon6,
    _on_draw_polygon7
};

void main(void) {
    _romdisk_init();

    dbgio_dev_default_init(DBGIO_DEV_VDP2_SIMPLE);
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

    // Determine whether to start automatically
    bool start_state = true;

    // Reset the VBLANK count in case initialization takes more than one frame
    _stats.vblank_count = 0;

    while (true) {
        smpc_peripheral_process();
        smpc_peripheral_digital_port(1, &_digital);

        if ((_digital.held.button.l) != 0) {
            start_state ^= true;
        }

        bool process_frame = start_state;

        if ((_digital.held.button.r) != 0) {
            process_frame = true;
        }

        if (process_frame) {
            dbgio_puts("[H[2J");

            const fix16_t frame_count = fix16_int32_from(_stats.frame_count);
            const fix16_t vblank_count = fix16_int32_from(_stats.vblank_count);

            cpu_divu_fix16_set(frame_count, vblank_count);

            const fix16_t quotient = cpu_divu_quotient_get();
            const fix16_t fps = fix16_int16_mul(quotient, 60);

            if (_stats.vblank_count > 1) {
                _stats.dropped_count++;
            }

            dbgio_printf("%i/%i -> %f (dropped frames: %i)\n", _stats.frame_count, _stats.vblank_count, fps, _stats.dropped_count);

            _stats.vblank_count = 0;
            _stats.frame_count = 0;

            scene::process_frame();

            dbgio_flush();
        }

        vdp_sync();

        _stats.frame_count++;
    }

    __builtin_unreachable();
}

void user_init(void) {
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_B,
                              VDP2_TVMD_VERT_240);

    vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE), COLOR_RGB1555(1, 7, 7, 7));

    vdp2_sprite_priority_set(0, 6);

    cpu_intc_mask_set(0);

    vdp2_tvmd_display_set();

    vdp1_sync_interval_set(VDP1_SYNC_INTERVAL_VARIABLE);

    vdp1_env_t vdp1_env;

    vdp1_env.erase_color = COLOR_RGB1555(0, 0, 0, 0);
    vdp1_env.erase_points[0].x = 0;
    vdp1_env.erase_points[0].y = 0;
    vdp1_env.erase_points[1].x = _screen_width - 1;
    vdp1_env.erase_points[1].y = _screen_height - 1;
    vdp1_env.bpp = VDP1_ENV_BPP_16;
    vdp1_env.rotation = VDP1_ENV_ROTATION_0;
    vdp1_env.color_mode = VDP1_ENV_COLOR_MODE_RGB_PALETTE;
    vdp1_env.sprite_type = 0;

    vdp1_env_set(&vdp1_env);

    vdp_sync_vblank_out_set(_vblank_out_handler);
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

    vdp1_vram_partitions_t vram_partitions;

    vdp1_vram_partitions_get(&vram_partitions);

    vdp1_cmdt_draw_mode_t polygon_draw_mode;
    polygon_draw_mode.raw = 0x0000;
    polygon_draw_mode.bits.pre_clipping_disable = true;
    polygon_draw_mode.bits.end_code_disable = true;
    polygon_draw_mode.bits.trans_pixel_disable = true;
    polygon_draw_mode.bits.hss_enable = true;

    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;

    vdp1_cmdt_draw_mode_t clear_draw_mode;
    clear_draw_mode.raw = 0x0000;
    clear_draw_mode.bits.hss_enable = true;
    clear_draw_mode.bits.pre_clipping_disable = true;
    clear_draw_mode.bits.end_code_disable = true;
    clear_draw_mode.bits.trans_pixel_disable = true;
    clear_draw_mode.bits.color_mode = 1;

    _scene_cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);
    assert(_scene_cmdt_list != nullptr);

    vdp1_cmdt_t* const cmdts = _scene_cmdt_list->cmdts;

    (void)memset(&cmdts[0], 0x00, ORDER_COUNT * sizeof(vdp1_cmdt));

    vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX], CMDT_VTX_SYSTEM_CLIP, &system_clip_coord);

    vdp1_cmdt_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_LOCAL_COORDS_INDEX], CMDT_VTX_LOCAL_COORD, &local_coord_ul);

    vdp1_cmdt_scaled_sprite_set(&cmdts[ORDER_ERASE_INDEX]);
    vdp1_cmdt_param_draw_mode_set(&cmdts[ORDER_ERASE_INDEX], clear_draw_mode);
    vdp1_cmdt_param_size_set(&cmdts[ORDER_ERASE_INDEX], 8, 1);
    vdp1_cmdt_param_char_base_set(&cmdts[ORDER_ERASE_INDEX], (vdp1_vram_t)vram_partitions.texture_base);
    vdp1_cmdt_param_color_mode1_set(&cmdts[ORDER_ERASE_INDEX], (vdp1_vram_t)vram_partitions.clut_base);

    /* The clear polygon is cut in half due to the VDP1 not having enough time
     * to clear the bottom half of the framebuffer in time */
    cmdts[ORDER_ERASE_INDEX].cmd_xa = 0;
    cmdts[ORDER_ERASE_INDEX].cmd_ya = ((_screen_height - 16) / 2) - 1;
    cmdts[ORDER_ERASE_INDEX].cmd_xb = 0;
    cmdts[ORDER_ERASE_INDEX].cmd_yb = 0;
    cmdts[ORDER_ERASE_INDEX].cmd_xc = _screen_width - 1;
    cmdts[ORDER_ERASE_INDEX].cmd_yc = _screen_height - 1;
    cmdts[ORDER_ERASE_INDEX].cmd_xd = 0;
    cmdts[ORDER_ERASE_INDEX].cmd_yd = 0;

    for (uint32_t i = ORDER_BUFFER_STARTING_INDEX; i < ORDER_BUFFER_END_INDEX; i++) {
        vdp1_cmdt_polygon_set(&cmdts[i]);
        vdp1_cmdt_param_draw_mode_set(&cmdts[i], polygon_draw_mode);
        vdp1_cmdt_param_color_bank_set(&cmdts[i], color_bank);
        cmdts[i].cmd_xa = 0;
        cmdts[i].cmd_ya = 0;
        cmdts[i].cmd_xb = 0;
        cmdts[i].cmd_yb = 0;
        cmdts[i].cmd_xc = 0;
        cmdts[i].cmd_yc = 0;
        cmdts[i].cmd_xd = 0;
        cmdts[i].cmd_yd = 0;
    }
}

static void _vblank_out_handler(void *) {
    _stats.vblank_count++;

    smpc_peripheral_intback_issue();
}

static void _on_start(uint32_t, bool) {
    // At the beginning of a processing frame, clear the previous Draw End
    // command
    const uint16_t end_index = ORDER_BUFFER_STARTING_INDEX +
        _cmdt_buffer_index;

    vdp1_cmdt* const end_cmdt = &_scene_cmdt_list->cmdts[end_index];

    end_cmdt->cmd_ctrl &= ~0x8000;

    _cmdt_buffer_index = 0;
}

static void _on_end(uint32_t frame_index, bool last_frame) {
    const uint32_t prev_buffer_index = _cmdt_buffer_index;

    const uint16_t end_index = ORDER_BUFFER_STARTING_INDEX +
        prev_buffer_index;

    vdp1_cmdt* const end_cmdt = &_scene_cmdt_list->cmdts[end_index];

    vdp1_cmdt_end_set(end_cmdt);

    _scene_cmdt_list->count = end_index + 1;

    vdp1_sync_cmdt_list_put(_scene_cmdt_list, 0, NULL, NULL);

    if (last_frame) {
        scene::reset();

        _stats.dropped_count = 0;
    }
}

static void _on_update_palette(uint8_t palette_index,
                               const scene::rgb444 color) {
    const uint8_t scaled_r = color.r << 2;
    const uint8_t scaled_g = color.g << 2;
    const uint8_t scaled_b = color.b << 2;

    const color_rgb1555_t rgb1555_color =
        COLOR_RGB1555(1, scaled_r, scaled_g, scaled_b);

    _palette[palette_index] = rgb1555_color;
}

static void _on_clear_screen(bool clear_screen) {
    vdp1_cmdt_t* const cmdts = _scene_cmdt_list->cmdts;

    if (!clear_screen) {
        vdp1_cmdt_jump_skip_assign(&cmdts[ORDER_ERASE_INDEX], ORDER_BUFFER_STARTING_INDEX << 2);

        vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);
    } else {
        vdp1_cmdt_jump_clear(&cmdts[ORDER_ERASE_INDEX]);

        vdp1_sync_mode_set(VDP1_SYNC_MODE_ERASE_CHANGE);
    }
}

static void _scale_vertices(uint8_vec2_t* const out_vertices,
                            uint8_vec2_t const * vertex_buffer,
                            size_t count) {
    for (uint32_t i = 0; i < count; i++) {
        out_vertices[i].x = vertex_buffer[i].x;
        out_vertices[i].y = vertex_buffer[i].y;

        // out_vertices[i].x = fix16_int32_to(fix16_int16_mul(_scale_width, vertex_buffer[i].x));
        // out_vertices[i].y = fix16_int32_to(fix16_int16_mul(_scale_height, vertex_buffer[i].y));
    }
}

static void  _triangle_vertex_set(vdp1_cmdt& cmdt,
                                     const uint8_vec2_t* vertices) {
    uint8_t* p = (uint8_t*)&cmdt.cmd_xa;

    p++;

    // cmdt.cmd_xa = vertices->x;
    // cmdt.cmd_ya = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // cmdt.cmd_xb = vertices->x;
    // cmdt.cmd_yb = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // cmdt.cmd_xc = vertices->x;
    // cmdt.cmd_yc = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices -= 2;

    // cmdt.cmd_xd = vertices->x;
    // cmdt.cmd_yd = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
}

static void  _quad_vertex_set(vdp1_cmdt& cmdt,
                                 const uint8_vec2_t* vertices) {
    uint8_t* p = (uint8_t*)&cmdt.cmd_xa;

    p++;

    // cmdt.cmd_xa = vertices->x;
    // cmdt.cmd_ya = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // cmdt.cmd_xb = vertices->x;
    // cmdt.cmd_yb = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // cmdt.cmd_xc = vertices->x;
    // cmdt.cmd_yc = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // cmdt.cmd_xd = vertices->x;
    // cmdt.cmd_yd = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
}

static void  _triangle_vertex_set(vdp1_cmdt& cmdt,
                                     const uint8_vec2_t* vertices,
                                     uint32_t start_index) {
    uint8_t* p = (uint8_t*)&cmdt.cmd_xa;

    p++;

    // cmdt.cmd_xa = vertices->x;
    // cmdt.cmd_ya = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    const uint8_vec2_t* next_vertices = vertices + start_index;

    // cmdt.cmd_xb = next_vertices->x;
    // cmdt.cmd_yb = next_vertices->y;
    *p = next_vertices->x;
    p += 2;
    *p = next_vertices->y;
    p += 2;

    next_vertices++;

    // cmdt.cmd_xc = next_vertices->x;
    // cmdt.cmd_yc = next_vertices->y;
    *p = next_vertices->x;
    p += 2;
    *p = next_vertices->y;
    p += 2;

    // cmdt.cmd_xd = vertices->x;
    // cmdt.cmd_yd = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
}

static void  _quad_vertex_set(vdp1_cmdt& cmdt,
                                 const uint8_vec2_t* vertices,
                                 uint32_t start_index) {
    uint8_t* p = (uint8_t*)&cmdt.cmd_xa;

    p++;

    // cmdt.cmd_xa = vertices->x;
    // cmdt.cmd_ya = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices += start_index;

    // cmdt.cmd_xb = vertices->x;
    // cmdt.cmd_yb = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // cmdt.cmd_xc = vertices->x;
    // cmdt.cmd_yc = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // cmdt.cmd_xd = vertices->x;
    // cmdt.cmd_yd = vertices->y;
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
}

static uint32_t _on_draw_polygon3(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  color_rgb1555_t color) {
    // uint8_vec2_t out_vertices[count];
    // _scale_vertices(out_vertices, vertex_buffer, count);

    cmdt[0].cmd_colr = color.raw;
    _triangle_vertex_set(cmdt[0], vertex_buffer);

    return 1;
}

static uint32_t _on_draw_polygon4(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  color_rgb1555_t color) {
    // uint8_vec2_t out_vertices[count];
    // _scale_vertices(out_vertices, vertex_buffer, count);

    cmdt[0].cmd_colr = color.raw;
    _quad_vertex_set(cmdt[0], vertex_buffer);

    return 1;
}

static uint32_t _on_draw_polygon5(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  color_rgb1555_t color) {
    // uint8_vec2_t out_vertices[count];
    // _scale_vertices(out_vertices, vertex_buffer, count);

    cmdt[0].cmd_colr = color.raw;
    _quad_vertex_set(cmdt[0], vertex_buffer);

    cmdt[1].cmd_colr = color.raw;
    _triangle_vertex_set(cmdt[1], vertex_buffer, 3);

    return 2;
}

static uint32_t _on_draw_polygon6(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  color_rgb1555_t color) {
    // uint8_vec2_t out_vertices[count];
    // _scale_vertices(out_vertices, vertex_buffer, count);

    cmdt[0].cmd_colr = color.raw;
    _quad_vertex_set(cmdt[0], vertex_buffer);

    cmdt[1].cmd_colr = color.raw;
    _quad_vertex_set(cmdt[1], vertex_buffer, 3);

    return 2;
}

static uint32_t _on_draw_polygon7(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  color_rgb1555_t color) {
    // uint8_vec2_t out_vertices[count];
    // _scale_vertices(out_vertices, vertex_buffer, count);

    cmdt[0].cmd_colr = color.raw;
    _quad_vertex_set(cmdt[0], vertex_buffer);

    cmdt[1].cmd_colr = color.raw;
    _quad_vertex_set(cmdt[1], vertex_buffer, 3);

    cmdt[2].cmd_colr = color.raw;
    _triangle_vertex_set(cmdt[2], vertex_buffer, 5);

    return 3;
}

static void _on_draw(const uint8_vec2_t* vertex_buffer,
                     uint32_t count,
                     uint32_t palette_index) {
    const draw_handler draw_handler = _draw_handlers[count];

    vdp1_cmdt* cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    const color_rgb1555_t color = _palette[palette_index];

    _cmdt_buffer_index += draw_handler(cmdt, vertex_buffer, color);
}

// #pragma GCC pop_options
