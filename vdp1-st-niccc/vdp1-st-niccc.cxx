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
#define ORDER_LOCAL_COORDS_INDEX            1
#define ORDER_ERASE_INDEX                   2
#define ORDER_BUFFER_STARTING_INDEX         3
#define ORDER_BUFFER_END_INDEX              (ORDER_BUFFER_STARTING_INDEX + 512)
#define ORDER_DRAW_END_INDEX                (ORDER_BUFFER_END_INDEX + 1)
#define ORDER_COUNT                         ORDER_DRAW_END_INDEX

static constexpr uint32_t _screen_width = 320;
static constexpr uint32_t _screen_height = 224;

static constexpr uint32_t _render_width = 256;
static constexpr uint32_t _render_height = 200;

static constexpr fix16_t _scale_width = FIX16(_screen_width / static_cast<float>(_render_width));
static constexpr fix16_t _scale_height = FIX16(_screen_height / static_cast<float>(_render_height));

static const char* _scene_file_path = "SCENE.BIN";

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
    .vblank_count = 0
};

static color_rgb1555_t _palette[16] __aligned(32);

static void _romdisk_init(void);

static void _draw_init(void);

static void _vblank_out_handler(void *);

static void _on_start(uint32_t, bool);
static void _on_end(uint32_t, bool);
static void _on_clear_screen(bool);
static void _on_update_palette(uint8_t, const scene::rgb444);
static void _on_draw(int16_vec2_t const *, const size_t, const uint8_t);

static void _on_draw_polygon3(int16_vec2_t const *, const size_t, const uint8_t);
static void _on_draw_polygon4(int16_vec2_t const *, const size_t, const uint8_t);
static void _on_draw_polygon5(int16_vec2_t const *, const size_t, const uint8_t);
static void _on_draw_polygon6(int16_vec2_t const *, const size_t, const uint8_t);
static void _on_draw_polygon7(int16_vec2_t const *, const size_t, const uint8_t);

static const scene::draw_handler _draw_handlers[] = {
    nullptr, // 0
    nullptr, // 1
    nullptr, // 2
    // nullptr,
    _on_draw_polygon3,
    // nullptr,
    _on_draw_polygon4,
    // nullptr,
    _on_draw_polygon5,
    // nullptr,
    _on_draw_polygon6,
    // nullptr,
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
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_224);

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
    vdp1_env.sprite_type = 0x0;

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

    vdp1_cmdt_draw_mode_t polygon_draw_mode;
    polygon_draw_mode.raw = 0x0000;
    polygon_draw_mode.bits.pre_clipping_disable = true;
    polygon_draw_mode.bits.end_code_disable = true;
    polygon_draw_mode.bits.trans_pixel_disable = true;

    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;

    _scene_cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);
    assert(_scene_cmdt_list != nullptr);

    vdp1_cmdt_t* const cmdts = _scene_cmdt_list->cmdts;

    (void)memset(&cmdts[0], 0x00, ORDER_COUNT * sizeof(vdp1_cmdt));

    vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX], CMDT_VTX_SYSTEM_CLIP, &system_clip_coord);

    vdp1_cmdt_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_LOCAL_COORDS_INDEX], CMDT_VTX_LOCAL_COORD, &local_coord_ul);

    vdp1_cmdt_polygon_set(&cmdts[ORDER_ERASE_INDEX]);
    vdp1_cmdt_jump_skip_assign(&cmdts[ORDER_ERASE_INDEX], ORDER_BUFFER_STARTING_INDEX << 2);
    vdp1_cmdt_param_draw_mode_set(&cmdts[ORDER_ERASE_INDEX], polygon_draw_mode);
    vdp1_cmdt_param_color_bank_set(&cmdts[ORDER_ERASE_INDEX], color_bank);
    vdp1_cmdt_param_color_set(&cmdts[ORDER_ERASE_INDEX], COLOR_RGB1555(1, 31, 0, 0));
    cmdts[ORDER_ERASE_INDEX].cmd_xa = 0;
    cmdts[ORDER_ERASE_INDEX].cmd_ya = 0;
    cmdts[ORDER_ERASE_INDEX].cmd_xb = _render_width - 1;
    cmdts[ORDER_ERASE_INDEX].cmd_yb = 0;
    cmdts[ORDER_ERASE_INDEX].cmd_xc = _render_width - 1;
    cmdts[ORDER_ERASE_INDEX].cmd_yc = _render_height - 1;
    cmdts[ORDER_ERASE_INDEX].cmd_xd = 0;
    cmdts[ORDER_ERASE_INDEX].cmd_yd = _render_height - 1;

    for (uint32_t i = ORDER_BUFFER_STARTING_INDEX; i < ORDER_BUFFER_END_INDEX; i++) {
        vdp1_cmdt_polygon_set(&cmdts[i]);
        vdp1_cmdt_param_draw_mode_set(&cmdts[i], polygon_draw_mode);
        vdp1_cmdt_param_color_bank_set(&cmdts[i], color_bank);
    }

    vdp1_cmdt_end_set(&cmdts[ORDER_BUFFER_STARTING_INDEX]);
}

static void _vblank_out_handler(void *) {
    _stats.vblank_count++;

    smpc_peripheral_intback_issue();
}

static void _on_start(uint32_t, bool) {
}

static void _on_end(uint32_t frame_index, bool last_frame) {
    const uint32_t prev_buffer_index = _cmdt_buffer_index;

    _cmdt_buffer_index = 0;

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
    if (!clear_screen) {
        vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);
    } else {
        vdp1_sync_mode_set(VDP1_SYNC_MODE_ERASE_CHANGE);
    }
}

static inline void _triangle_vertex_set(vdp1_cmdt& cmdt,
                                        const int16_vec2_t* vertices) {
    cmdt.cmd_xa = vertices->x;
    cmdt.cmd_ya = vertices->y;

    vertices++;

    cmdt.cmd_xb = vertices->x;
    cmdt.cmd_yb = vertices->y;

    vertices++;

    cmdt.cmd_xc = vertices->x;
    cmdt.cmd_yc = vertices->y;

    vertices -= 2;

    cmdt.cmd_xd = vertices->x;
    cmdt.cmd_yd = vertices->y;
}

static inline void _quad_vertex_set(vdp1_cmdt& cmdt,
                                    const int16_vec2_t* vertices) {
    cmdt.cmd_xa = vertices->x;
    cmdt.cmd_ya = vertices->y;

    vertices++;

    cmdt.cmd_xb = vertices->x;
    cmdt.cmd_yb = vertices->y;

    vertices++;

    cmdt.cmd_xc = vertices->x;
    cmdt.cmd_yc = vertices->y;

    vertices++;

    cmdt.cmd_xd = vertices->x;
    cmdt.cmd_yd = vertices->y;
}

static inline void _triangle_vertex_set(vdp1_cmdt& cmdt,
                                        const int16_vec2_t* vertices,
                                        uint32_t start_index) {
    cmdt.cmd_xa = vertices->x;
    cmdt.cmd_ya = vertices->y;

    const int16_vec2_t* next_vertices = vertices + start_index;

    cmdt.cmd_xb = next_vertices->x;
    cmdt.cmd_yb = next_vertices->y;

    next_vertices++;

    cmdt.cmd_xc = next_vertices->x;
    cmdt.cmd_yc = next_vertices->y;

    cmdt.cmd_xd = vertices->x;
    cmdt.cmd_yd = vertices->y;
}

static inline void _quad_vertex_set(vdp1_cmdt& cmdt,
                                    const int16_vec2_t* vertices,
                                    uint32_t start_index) {
    cmdt.cmd_xa = vertices->x;
    cmdt.cmd_ya = vertices->y;

    vertices += start_index;

    cmdt.cmd_xb = vertices->x;
    cmdt.cmd_yb = vertices->y;

    vertices++;

    cmdt.cmd_xc = vertices->x;
    cmdt.cmd_yc = vertices->y;

    vertices++;

    cmdt.cmd_xd = vertices->x;
    cmdt.cmd_yd = vertices->y;
}

static void _scale_vertices(int16_vec2_t* const out_vertices,
                            int16_vec2_t const * vertex_buffer,
                            size_t count) {
    for (uint32_t i = 0; i < count; i++) {
        out_vertices[i].x = vertex_buffer[i].x;
        out_vertices[i].y = vertex_buffer[i].y;

        // out_vertices[i].x = fix16_int32_to(fix16_int16_mul(_scale_width, vertex_buffer[i].x));
        // out_vertices[i].y = fix16_int32_to(fix16_int16_mul(_scale_height, vertex_buffer[i].y));
    }
}

static void _prepare_cmdts(vdp1_cmdt* const cmdts,
                           size_t cmdt_count,
                           uint8_t palette_index) {
    const color_rgb1555_t color = _palette[palette_index];

    for (uint32_t i = 0; i < cmdt_count; i++) {
        vdp1_cmdt_polygon_set(&cmdts[i]);
        vdp1_cmdt_param_color_set(&cmdts[i], color);
    }
}

static void _on_draw_polygon3(int16_vec2_t const * vertex_buffer,
                              const size_t count,
                              const uint8_t palette_index) {
    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    // int16_vec2_t out_vertices[count];

    _prepare_cmdts(cmdt, 1, palette_index);
    // _scale_vertices(out_vertices, vertex_buffer, count);

    _triangle_vertex_set(cmdt[0], vertex_buffer);

    _cmdt_buffer_index += 1;
}

static void _on_draw_polygon4(int16_vec2_t const * vertex_buffer,
                              size_t count,
                              uint8_t palette_index) {
    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    // int16_vec2_t out_vertices[count];

    _prepare_cmdts(cmdt, 1, palette_index);
    // _scale_vertices(out_vertices, vertex_buffer, count);

    _quad_vertex_set(cmdt[0], vertex_buffer);

    _cmdt_buffer_index += 1;
}

static void _on_draw_polygon5(int16_vec2_t const * vertex_buffer,
                              size_t count,
                              uint8_t palette_index) {
    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    // int16_vec2_t out_vertices[count];

    _prepare_cmdts(cmdt, 2, palette_index);
    // _scale_vertices(out_vertices, vertex_buffer, count);

    _quad_vertex_set(cmdt[0], vertex_buffer);
    _triangle_vertex_set(cmdt[1], vertex_buffer, 3);

    _cmdt_buffer_index += 2;
}

static void _on_draw_polygon6(int16_vec2_t const * vertex_buffer,
                              size_t count,
                              uint8_t palette_index) {
    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    // int16_vec2_t out_vertices[count];

    _prepare_cmdts(cmdt, 2, palette_index);
    // _scale_vertices(out_vertices, vertex_buffer, count);

    _quad_vertex_set(cmdt[0], vertex_buffer);
    _quad_vertex_set(cmdt[1], vertex_buffer, 3);

    _cmdt_buffer_index += 2;
}

static void _on_draw_polygon7(int16_vec2_t const * vertex_buffer,
                              size_t count,
                              uint8_t palette_index) {
    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    // int16_vec2_t out_vertices[count];

    _prepare_cmdts(cmdt, 3, palette_index);
    // _scale_vertices(out_vertices, vertex_buffer, count);

    _quad_vertex_set(cmdt[0], vertex_buffer);
    _quad_vertex_set(cmdt[1], vertex_buffer, 3);
    _triangle_vertex_set(cmdt[2], vertex_buffer, 5);

    _cmdt_buffer_index += 3;
}

static void _on_draw(int16_vec2_t const * vertex_buffer,
                     size_t count,
                     uint8_t palette_index) {
        const scene::draw_handler draw_handler = _draw_handlers[count];

        if (draw_handler != nullptr) {
                draw_handler(vertex_buffer, count, palette_index);
        }
}
