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
#define ORDER_BUFFER_STARTING_INDEX         2
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

static smpc_peripheral_digital_t _digital;

static vdp1_cmdt_list_t* _scene_cmdt_list;
static uint32_t _cmdt_buffer_index;

static color_rgb1555_t _palette[16] __aligned(32);

static void _hardware_init(void);
static void _romdisk_init(void);

static void _draw_init(void);

static void _vblank_out_handler(void *);

static void _on_start(uint32_t, bool);
static void _on_end(uint32_t, bool);
static void _on_clear_screen(bool);
static void _on_update_palette(uint8_t, const scene::rgb444);
static void _on_draw(int16_vec2_t const *, const size_t, const uint8_t);

static void _on_draw_polygon3(int16_vec2_t const *, const size_t, const uint8_t) __unused;
static void _on_draw_polygon4(int16_vec2_t const *, const size_t, const uint8_t) __unused;
static void _on_draw_polygon5(int16_vec2_t const *, const size_t, const uint8_t) __unused;
static void _on_draw_polygon6(int16_vec2_t const *, const size_t, const uint8_t) __unused;
static void _on_draw_polygon7(int16_vec2_t const *, const size_t, const uint8_t) __unused;

static const scene::draw_handler _draw_handlers[] = {
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

    bool start_state = false;

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

            scene::process_frame();

            dbgio_flush();
        }

        vdp_sync();
    }

    __builtin_unreachable();
}

static void _hardware_init(void) {
    cpu_cache_disable();

    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                              VDP2_TVMD_VERT_240);

    vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE), COLOR_RGB1555(1, 7, 7, 7));

    vdp2_sprite_priority_set(0, 6);

    cpu_intc_mask_set(0);

    vdp2_tvmd_display_set();

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

    _scene_cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);

    vdp1_cmdt_t* const cmdts = _scene_cmdt_list->cmdts;

    (void)memset(&cmdts[0], 0x00, ORDER_COUNT * sizeof(vdp1_cmdt));

    vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX], CMDT_VTX_SYSTEM_CLIP, &system_clip_coord);

    vdp1_cmdt_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[ORDER_LOCAL_COORDS_INDEX], CMDT_VTX_LOCAL_COORD, &local_coord_ul);

    for (uint32_t i = ORDER_BUFFER_STARTING_INDEX; i < ORDER_BUFFER_END_INDEX; i++) {
        vdp1_cmdt_param_draw_mode_set(&cmdts[i], polygon_draw_mode);
    }

    vdp1_cmdt_end_set(&cmdts[ORDER_BUFFER_STARTING_INDEX]);
}

static void _vblank_out_handler(void *) {
    smpc_peripheral_intback_issue();
}

static void _on_start(uint32_t, bool) {
}

static void _on_end(uint32_t frame_index, bool last_frame) {
    const uint32_t prev_buffer_index = _cmdt_buffer_index;

    _cmdt_buffer_index = 0;

    dbgio_printf("i: %li, frame_index: %li\n", prev_buffer_index, frame_index);

    for (uint32_t i = 0; i < 28; i++) {
        dbgio_printf("%3li. 0x%03X: 0x%04X\n", i, (uint16_t)(i << 5), MEMORY_READ(16, VDP1_VRAM(i << 5)));
    }

    const uint16_t end_index = ORDER_BUFFER_STARTING_INDEX +
        prev_buffer_index;

    vdp1_cmdt* const end_cmdt = &_scene_cmdt_list->cmdts[end_index];

    vdp1_cmdt_end_set(end_cmdt);

    _scene_cmdt_list->count = end_index + 1;

    vdp1_sync_cmdt_list_put(_scene_cmdt_list, NULL, NULL);

    if (last_frame) {
        scene::reset();
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

    (void)memcpy((void*)VDP2_CRAM_ADDR(0x10), &_palette[0], sizeof(_palette));
}

static void _on_clear_screen(bool clear_screen __unused) {
    if (!clear_screen) {
        vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);
    } else {
        vdp1_sync_mode_set(VDP1_SYNC_MODE_ERASE_CHANGE);
    }
}

static inline __always_inline void _vertex_set(int16_t& x,
                                               int16_t& y,
                                               int16_vec2_t const& vertex) {
    x = vertex.x;
    y = vertex.y;
    // x = fix16_int16_mul(_scale_width, vertex.x) >> 16;
    // y = fix16_int16_mul(_scale_height, vertex.y) >> 16;
}

static void _on_draw_polygon3(int16_vec2_t const * vertex_buffer __unused,
                              const size_t count __unused,
                              const uint8_t palette_index __unused) {
    /* Specify the CRAM offset */
    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;
    color_bank.type_0.data.dc = palette_index + 0x10;

    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    vdp1_cmdt_polygon_set(cmdt);
    vdp1_cmdt_param_color_bank_set(cmdt, color_bank);

    int16_vec2_t out_v[3];

    _vertex_set(out_v[0].x, out_v[0].y, vertex_buffer[0]);
    _vertex_set(out_v[1].x, out_v[1].y, vertex_buffer[1]);
    _vertex_set(out_v[2].x, out_v[2].y, vertex_buffer[2]);

    cmdt->cmd_xa = out_v[0].x;
    cmdt->cmd_ya = out_v[0].y;

    cmdt->cmd_xb = out_v[1].x;
    cmdt->cmd_yb = out_v[1].y;

    cmdt->cmd_xc = out_v[1].x;
    cmdt->cmd_yc = out_v[1].y;

    cmdt->cmd_xd = out_v[2].x;
    cmdt->cmd_yd = out_v[2].y;

    _cmdt_buffer_index++;
}

static void _on_draw_polygon4(int16_vec2_t const * vertex_buffer __unused,
                              size_t count __unused,
                              uint8_t palette_index __unused) {
    /* Specify the CRAM offset */
    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;
    color_bank.type_0.data.dc = palette_index + 0x10;

    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    vdp1_cmdt_polygon_set(cmdt);
    vdp1_cmdt_param_color_bank_set(cmdt, color_bank);

    _vertex_set(cmdt->cmd_xa, cmdt->cmd_ya, vertex_buffer[0]);
    _vertex_set(cmdt->cmd_xb, cmdt->cmd_yb, vertex_buffer[1]);
    _vertex_set(cmdt->cmd_xc, cmdt->cmd_yc, vertex_buffer[2]);
    _vertex_set(cmdt->cmd_xd, cmdt->cmd_yd, vertex_buffer[3]);

    _cmdt_buffer_index++;
}

static void _on_draw_polygon5(int16_vec2_t const * vertex_buffer __unused,
                              size_t count __unused,
                              uint8_t palette_index __unused) {
    /* Specify the CRAM offset */
    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;
    color_bank.type_0.data.dc = palette_index + 0x10;

    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    vdp1_cmdt_polygon_set(&cmdt[0]);
    vdp1_cmdt_param_color_bank_set(&cmdt[0], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[1]);
    vdp1_cmdt_param_color_bank_set(&cmdt[1], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[2]);
    vdp1_cmdt_param_color_bank_set(&cmdt[2], color_bank);

    int16_vec2_t out_v[5];

    _vertex_set(out_v[0].x, out_v[0].y, vertex_buffer[0]);
    _vertex_set(out_v[1].x, out_v[1].y, vertex_buffer[1]);
    _vertex_set(out_v[2].x, out_v[2].y, vertex_buffer[2]);
    _vertex_set(out_v[3].x, out_v[3].y, vertex_buffer[3]);
    _vertex_set(out_v[4].x, out_v[4].y, vertex_buffer[4]);

    // vertex_buffer[0]
    // vertex_buffer[1]
    // vertex_buffer[1]
    // vertex_buffer[2]
    cmdt[0].cmd_xa = out_v[0].x;
    cmdt[0].cmd_ya = out_v[0].y;
    cmdt[0].cmd_xb = out_v[1].x;
    cmdt[0].cmd_yb = out_v[1].y;
    cmdt[0].cmd_xc = out_v[1].x;
    cmdt[0].cmd_yc = out_v[1].y;
    cmdt[0].cmd_xd = out_v[2].x;
    cmdt[0].cmd_yd = out_v[2].y;

    // vertex_buffer[0]
    // vertex_buffer[2]
    // vertex_buffer[2]
    // vertex_buffer[3]
    cmdt[1].cmd_xa = out_v[0].x;
    cmdt[1].cmd_ya = out_v[0].y;
    cmdt[1].cmd_xb = out_v[2].x;
    cmdt[1].cmd_yb = out_v[2].y;
    cmdt[1].cmd_xc = out_v[2].x;
    cmdt[1].cmd_yc = out_v[2].y;
    cmdt[1].cmd_xd = out_v[3].x;
    cmdt[1].cmd_yd = out_v[3].y;

    // vertex_buffer[0]
    // vertex_buffer[3]
    // vertex_buffer[3]
    // vertex_buffer[4]
    cmdt[2].cmd_xa = out_v[0].x;
    cmdt[2].cmd_ya = out_v[0].y;
    cmdt[2].cmd_xb = out_v[3].x;
    cmdt[2].cmd_yb = out_v[3].y;
    cmdt[2].cmd_xc = out_v[3].x;
    cmdt[2].cmd_yc = out_v[3].y;
    cmdt[2].cmd_xd = out_v[4].x;
    cmdt[2].cmd_yd = out_v[4].y;

    _cmdt_buffer_index += 3;
}

static void _on_draw_polygon6(int16_vec2_t const * vertex_buffer __unused,
                              size_t count __unused,
                              uint8_t palette_index __unused) {
    /* Specify the CRAM offset */
    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;
    color_bank.type_0.data.dc = palette_index + 0x10;

    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    vdp1_cmdt_polygon_set(&cmdt[0]);
    vdp1_cmdt_param_color_bank_set(&cmdt[0], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[1]);
    vdp1_cmdt_param_color_bank_set(&cmdt[1], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[2]);
    vdp1_cmdt_param_color_bank_set(&cmdt[2], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[3]);
    vdp1_cmdt_param_color_bank_set(&cmdt[3], color_bank);

    int16_vec2_t out_v[6];

    _vertex_set(out_v[0].x, out_v[0].y, vertex_buffer[0]);
    _vertex_set(out_v[1].x, out_v[1].y, vertex_buffer[1]);
    _vertex_set(out_v[2].x, out_v[2].y, vertex_buffer[2]);
    _vertex_set(out_v[3].x, out_v[3].y, vertex_buffer[3]);
    _vertex_set(out_v[4].x, out_v[4].y, vertex_buffer[4]);
    _vertex_set(out_v[5].x, out_v[5].y, vertex_buffer[5]);

    // vertex_buffer[0]
    // vertex_buffer[1]
    // vertex_buffer[1]
    // vertex_buffer[2]
    cmdt[0].cmd_xa = out_v[0].x;
    cmdt[0].cmd_ya = out_v[0].y;
    cmdt[0].cmd_xb = out_v[1].x;
    cmdt[0].cmd_yb = out_v[1].y;
    cmdt[0].cmd_xc = out_v[1].x;
    cmdt[0].cmd_yc = out_v[1].y;
    cmdt[0].cmd_xd = out_v[2].x;
    cmdt[0].cmd_yd = out_v[2].y;

    // vertex_buffer[0]
    // vertex_buffer[2]
    // vertex_buffer[2]
    // vertex_buffer[3]
    cmdt[1].cmd_xa = out_v[0].x;
    cmdt[1].cmd_ya = out_v[0].y;
    cmdt[1].cmd_xb = out_v[2].x;
    cmdt[1].cmd_yb = out_v[2].y;
    cmdt[1].cmd_xc = out_v[2].x;
    cmdt[1].cmd_yc = out_v[2].y;
    cmdt[1].cmd_xd = out_v[3].x;
    cmdt[1].cmd_yd = out_v[3].y;

    // vertex_buffer[0]
    // vertex_buffer[3]
    // vertex_buffer[3]
    // vertex_buffer[4]
    cmdt[2].cmd_xa = out_v[0].x;
    cmdt[2].cmd_ya = out_v[0].y;
    cmdt[2].cmd_xb = out_v[3].x;
    cmdt[2].cmd_yb = out_v[3].y;
    cmdt[2].cmd_xc = out_v[3].x;
    cmdt[2].cmd_yc = out_v[3].y;
    cmdt[2].cmd_xd = out_v[4].x;
    cmdt[2].cmd_yd = out_v[4].y;

    // vertex_buffer[0]
    // vertex_buffer[4]
    // vertex_buffer[4]
    // vertex_buffer[5]
    cmdt[3].cmd_xa = out_v[0].x;
    cmdt[3].cmd_ya = out_v[0].y;
    cmdt[3].cmd_xb = out_v[4].x;
    cmdt[3].cmd_yb = out_v[4].y;
    cmdt[3].cmd_xc = out_v[4].x;
    cmdt[3].cmd_yc = out_v[4].y;
    cmdt[3].cmd_xd = out_v[5].x;
    cmdt[3].cmd_yd = out_v[5].y;

    _cmdt_buffer_index += 4;
}

static void _on_draw_polygon7(int16_vec2_t const * vertex_buffer __unused,
                              size_t count __unused,
                              uint8_t palette_index __unused) {
    /* Specify the CRAM offset */
    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;
    color_bank.type_0.data.dc = palette_index + 0x10;

    vdp1_cmdt* const cmdt =
        &_scene_cmdt_list->cmdts[ORDER_BUFFER_STARTING_INDEX + _cmdt_buffer_index];

    vdp1_cmdt_polygon_set(&cmdt[0]);
    vdp1_cmdt_param_color_bank_set(&cmdt[0], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[1]);
    vdp1_cmdt_param_color_bank_set(&cmdt[1], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[2]);
    vdp1_cmdt_param_color_bank_set(&cmdt[2], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[3]);
    vdp1_cmdt_param_color_bank_set(&cmdt[3], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[4]);
    vdp1_cmdt_param_color_bank_set(&cmdt[4], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[5]);
    vdp1_cmdt_param_color_bank_set(&cmdt[5], color_bank);
    vdp1_cmdt_polygon_set(&cmdt[6]);
    vdp1_cmdt_param_color_bank_set(&cmdt[6], color_bank);

    int16_vec2_t out_v[7];

    _vertex_set(out_v[0].x, out_v[0].y, vertex_buffer[0]);
    _vertex_set(out_v[1].x, out_v[1].y, vertex_buffer[1]);
    _vertex_set(out_v[2].x, out_v[2].y, vertex_buffer[2]);
    _vertex_set(out_v[3].x, out_v[3].y, vertex_buffer[3]);
    _vertex_set(out_v[4].x, out_v[4].y, vertex_buffer[4]);
    _vertex_set(out_v[5].x, out_v[5].y, vertex_buffer[5]);
    _vertex_set(out_v[6].x, out_v[6].y, vertex_buffer[6]);

    // vertex_buffer[0]
    // vertex_buffer[1]
    // vertex_buffer[1]
    // vertex_buffer[2]
    cmdt[0].cmd_xa = out_v[0].x;
    cmdt[0].cmd_ya = out_v[0].y;
    cmdt[0].cmd_xb = out_v[1].x;
    cmdt[0].cmd_yb = out_v[1].y;
    cmdt[0].cmd_xc = out_v[1].x;
    cmdt[0].cmd_yc = out_v[1].y;
    cmdt[0].cmd_xd = out_v[2].x;
    cmdt[0].cmd_yd = out_v[2].y;

    // vertex_buffer[0]
    // vertex_buffer[2]
    // vertex_buffer[2]
    // vertex_buffer[3]
    cmdt[1].cmd_xa = out_v[0].x;
    cmdt[1].cmd_ya = out_v[0].y;
    cmdt[1].cmd_xb = out_v[2].x;
    cmdt[1].cmd_yb = out_v[2].y;
    cmdt[1].cmd_xc = out_v[2].x;
    cmdt[1].cmd_yc = out_v[2].y;
    cmdt[1].cmd_xd = out_v[3].x;
    cmdt[1].cmd_yd = out_v[3].y;

    // vertex_buffer[0]
    // vertex_buffer[3]
    // vertex_buffer[3]
    // vertex_buffer[4]
    cmdt[2].cmd_xa = out_v[0].x;
    cmdt[2].cmd_ya = out_v[0].y;
    cmdt[2].cmd_xb = out_v[3].x;
    cmdt[2].cmd_yb = out_v[3].y;
    cmdt[2].cmd_xc = out_v[3].x;
    cmdt[2].cmd_yc = out_v[3].y;
    cmdt[2].cmd_xd = out_v[4].x;
    cmdt[2].cmd_yd = out_v[4].y;

    // vertex_buffer[0]
    // vertex_buffer[4]
    // vertex_buffer[4]
    // vertex_buffer[5]
    cmdt[3].cmd_xa = out_v[0].x;
    cmdt[3].cmd_ya = out_v[0].y;
    cmdt[3].cmd_xb = out_v[4].x;
    cmdt[3].cmd_yb = out_v[4].y;
    cmdt[3].cmd_xc = out_v[4].x;
    cmdt[3].cmd_yc = out_v[4].y;
    cmdt[3].cmd_xd = out_v[5].x;
    cmdt[3].cmd_yd = out_v[5].y;

    // vertex_buffer[0]
    // vertex_buffer[5]
    // vertex_buffer[5]
    // vertex_buffer[6]
    cmdt[4].cmd_xa = out_v[0].x;
    cmdt[4].cmd_ya = out_v[0].y;
    cmdt[4].cmd_xb = out_v[5].x;
    cmdt[4].cmd_yb = out_v[5].y;
    cmdt[4].cmd_xc = out_v[5].x;
    cmdt[4].cmd_yc = out_v[5].y;
    cmdt[4].cmd_xd = out_v[6].x;
    cmdt[4].cmd_yd = out_v[6].y;

    _cmdt_buffer_index += 5;
}

static void _on_draw(int16_vec2_t const * vertex_buffer,
                     size_t count,
                     uint8_t palette_index) {
    _draw_handlers[count](vertex_buffer, count, palette_index);
}
