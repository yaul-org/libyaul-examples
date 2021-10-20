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

#define VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX    0
#define VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX          1
#define VDP1_CMDT_ORDER_ERASE_INDEX                 2
#define VDP1_CMDT_ORDER_BUFFER_STARTING_INDEX       3
#define VDP1_CMDT_ORDER_BUFFER_END_INDEX            (VDP1_CMDT_ORDER_BUFFER_STARTING_INDEX + 512)
#define VDP1_CMDT_ORDER_DRAW_END_INDEX              (VDP1_CMDT_ORDER_BUFFER_END_INDEX + 1)
#define VDP1_CMDT_ORDER_COUNT                       VDP1_CMDT_ORDER_DRAW_END_INDEX

#define RBG0_BPD            VDP2_VRAM_ADDR(0, 0x000000)
#define RBG0_ROTATION_TABLE VDP2_VRAM_ADDR(2, 0x000000)

#define BACK_SCREEN         VDP2_VRAM_ADDR(1, 0x01FFFE)

typedef uint32_t (*draw_handler)(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, vdp1_cmdt_color_bank_t color_bank);

static constexpr uint32_t _screen_width = 352;
static constexpr uint32_t _screen_height = 240;

static constexpr uint32_t _render_width = 256;
static constexpr uint32_t _render_height = 200;

static constexpr fix16_t _scale_width = FIX16(_render_width / static_cast<float>(_screen_width));
static constexpr fix16_t _scale_height = FIX16(_render_height / static_cast<float>(_screen_height));

static const char* _scene_file_path = "SCENE.BIN";

static void* _romdisk;

static smpc_peripheral_digital_t _digital;

static struct {
    vdp1_cmdt_list_t* cmdt_lists[2];
    uint32_t which_cmdt_list;

    vdp1_cmdt_list_t* cmdt_list;

    color_rgb1555_t palette[16];
} __aligned(16) _scene;

static volatile struct {
    uint32_t frame_count;
    uint32_t vblank_count;
    uint32_t dropped_count;
    uint32_t ovi_count;
} _stats = {
    .frame_count   = 0,
    .vblank_count  = 0,
    .dropped_count = 0,
    .ovi_count     = 0
};

static void _romdisk_init(void);

static void _draw_init(void);

static void _vblank_out_handler(void *);

static void _frt_ovi_handler(void);

static void _on_start(uint32_t, bool);
static void _on_end(uint32_t, bool);
static void _on_clear_screen(bool);
static void _on_update_palette(uint8_t, const scene::rgb444);
static void _on_draw(const uint8_vec2_t *, uint32_t, uint32_t);

static uint32_t _on_draw_polygon3(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, vdp1_cmdt_color_bank_t color_bank);
static uint32_t _on_draw_polygon4(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, vdp1_cmdt_color_bank_t color_bank);
static uint32_t _on_draw_polygon5(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, vdp1_cmdt_color_bank_t color_bank);
static uint32_t _on_draw_polygon6(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, vdp1_cmdt_color_bank_t color_bank);
static uint32_t _on_draw_polygon7(vdp1_cmdt* cmdt, const uint8_vec2_t* vertex_buffer, vdp1_cmdt_color_bank_t color_bank);

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

int main(void) {
    _romdisk_init();

    dbgio_dev_default_init(DBGIO_DEV_VDP2);
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

    dbgio_flush();
    vdp2_sync();
    vdp2_sync_wait();

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
            if (_stats.vblank_count > 1) {
                _stats.dropped_count++;
            }

            dbgio_printf("[H[2J\n_stats.dropped_count: %i\n", _stats.dropped_count);

            _stats.vblank_count = 0;
            _stats.frame_count = 0;

            dbgio_flush();
            vdp2_sync();

            scene::process_frame();
        } else {
                vdp2_tvmd_vblank_in_wait();
                vdp2_tvmd_vblank_out_wait();
        }

        _stats.frame_count++;
    }

    __builtin_unreachable();
}

static void _vdp1_init(void) {
    vdp2_sprite_priority_set(0, 6);

    vdp1_sync_interval_set(VDP1_SYNC_INTERVAL_VARIABLE);

    vdp1_env_t vdp1_env;

    vdp1_env.erase_color = COLOR_RGB1555(0, 0, 0, 0);
    vdp1_env.erase_points[0].x = 0;
    vdp1_env.erase_points[0].y = 0;
    vdp1_env.erase_points[1].x = _render_width - 1;
    vdp1_env.erase_points[1].y = _render_height - 1;
    vdp1_env.bpp = VDP1_ENV_BPP_8;
    vdp1_env.rotation = VDP1_ENV_ROTATION_90;
    vdp1_env.color_mode = VDP1_ENV_COLOR_MODE_PALETTE;
    vdp1_env.sprite_type = 8;

    vdp1_env_set(&vdp1_env);
}

static void _vdp2_init(void) {
    static const vdp2_scrn_bitmap_format_t rbg0_format = {
            .scroll_screen       = VDP2_SCRN_RBG0,
            .cc_count            = VDP2_SCRN_CCC_PALETTE_256,
            .bitmap_size         = { 512, 512 },
            .color_palette       = 0x00000000,
            .bitmap_pattern      = RBG0_BPD,
            .sf_type             = VDP2_SCRN_SF_TYPE_NONE,
            .sf_code             = VDP2_SCRN_SF_CODE_A,
            .sf_mode             = 0,
            .rp_mode             = 0, /* Mode 0: Rotation Parameter A */
            .rotation_table_base = RBG0_ROTATION_TABLE,
            .usage_banks         = {
                    .a0 = VDP2_VRAM_USAGE_TYPE_BPD,
                    .a1 = VDP2_VRAM_USAGE_TYPE_NONE,
                    .b0 = VDP2_VRAM_USAGE_TYPE_NONE,
                    .b1 = VDP2_VRAM_USAGE_TYPE_NONE
            }
    };

    static const vdp2_scrn_rotation_table_t rbg0_rotation_table __used = {
            // Starting TV screen coordinates
            .xst = 0,
            .yst = 0,
            .zst = 0,

            // Screen vertical coordinate increments (per each line)
            .delta_xst = FIX16(0.0f),
            .delta_yst = _scale_height,

            // Screen horizontal coordinate increments (per each dot)
            .delta_x = _scale_width,
            .delta_y = FIX16(0.0f),

            .matrix = {
                    .param = {
                            .a = FIX16(1.0f),
                            .b = FIX16(0.0f),
                            .c = FIX16(0.0f),
                            .d = FIX16(0.0f),
                            .e = FIX16(1.0f),
                            .f = FIX16(0.0f)
                    }
            },

            // View point coordinates
            .px = 0,
            .py = 0,
            .pz = 0,

            // Center coordinates
            .cx = 0,
            .cy = 0,
            .cz = 0,

            // Amount of horizontal shifting
            .mx = 0,
            .my = 0,

            // Scaling coefficients
            .kx = FIX16(1.0f),
            .ky = FIX16(1.0f),

            // Coefficient table start address
            .kast = 0,
            // Addr. increment coeff. table (per line)
            .delta_kast = 0,
            // Addr. increment coeff. table (per dot)
            .delta_kax = 0,
    };

    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_B,
                              VDP2_TVMD_VERT_240);

    vdp2_scrn_back_screen_color_set(BACK_SCREEN, COLOR_RGB1555(1, 7, 7, 7));

    vdp2_scrn_bitmap_format_set(&rbg0_format);
    vdp2_scrn_priority_set(VDP2_SCRN_RBG0, 7);
    vdp2_scrn_display_set(VDP2_SCRN_RBG0, /* no_trans = */ true);

    (void)memcpy((void *)RBG0_ROTATION_TABLE, &rbg0_rotation_table, sizeof(rbg0_rotation_table));

    vdp2_tvmd_display_set();
}

void user_init(void) {
    cpu_intc_mask_set(0);

    _vdp2_init();
    _vdp1_init();

    vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

    cpu_frt_init(CPU_FRT_CLOCK_DIV_8);
    cpu_frt_ovi_set(_frt_ovi_handler);
}

static void _romdisk_init(void) {
    extern uint8_t root_romdisk[];

    romdisk_init();

    _romdisk = romdisk_mount("/", root_romdisk);
    assert(_romdisk != NULL);
}

static void _draw_cmdt_list_init(vdp1_cmdt_list_t* cmdt_list) {
    constexpr int16_vec2_t system_clip_coord =
        INT16_VEC2_INITIALIZER(_render_width - 1,
                               _render_height - 1);

    constexpr int16_vec2_t local_coord_ul =
        INT16_VEC2_INITIALIZER(0,
                               0);

    vdp1_vram_partitions_t vram_partitions;

    vdp1_vram_partitions_get(&vram_partitions);

    vdp1_cmdt_draw_mode_t polygon_draw_mode;
    polygon_draw_mode.raw = 0x0000;
    polygon_draw_mode.bits.hss_enable = true;
    polygon_draw_mode.bits.pre_clipping_disable = true;
    polygon_draw_mode.bits.end_code_disable = true;
    polygon_draw_mode.bits.trans_pixel_disable = true;

    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;

    vdp1_cmdt_draw_mode_t clear_draw_mode;
    clear_draw_mode.raw = 0x0000;
    clear_draw_mode.bits.hss_enable = true;
    clear_draw_mode.bits.pre_clipping_disable = true;
    clear_draw_mode.bits.end_code_disable = true;
    clear_draw_mode.bits.trans_pixel_disable = true;
    clear_draw_mode.bits.color_mode = 1;

    vdp1_cmdt_t* const cmdts = cmdt_list->cmdts;

    (void)memset(&cmdts[0], 0x00, VDP1_CMDT_ORDER_COUNT * sizeof(vdp1_cmdt));

    vdp1_cmdt_system_clip_coord_set(&cmdts[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX], CMDT_VTX_SYSTEM_CLIP, &system_clip_coord);

    vdp1_cmdt_local_coord_set(&cmdts[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX]);
    vdp1_cmdt_param_vertex_set(&cmdts[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX], CMDT_VTX_LOCAL_COORD, &local_coord_ul);

    vdp1_cmdt_scaled_sprite_set(&cmdts[VDP1_CMDT_ORDER_ERASE_INDEX]);
    vdp1_cmdt_jump_skip_assign(&cmdts[VDP1_CMDT_ORDER_ERASE_INDEX], VDP1_CMDT_ORDER_BUFFER_STARTING_INDEX);
    vdp1_cmdt_param_draw_mode_set(&cmdts[VDP1_CMDT_ORDER_ERASE_INDEX], clear_draw_mode);
    vdp1_cmdt_param_size_set(&cmdts[VDP1_CMDT_ORDER_ERASE_INDEX], 8, 1);
    vdp1_cmdt_param_char_base_set(&cmdts[VDP1_CMDT_ORDER_ERASE_INDEX], (vdp1_vram_t)vram_partitions.texture_base);
    vdp1_cmdt_param_color_mode1_set(&cmdts[VDP1_CMDT_ORDER_ERASE_INDEX], (vdp1_vram_t)vram_partitions.clut_base);

    // The clear polygon is cut in half due to the VDP1 not having enough time *
    // to clear the bottom half of the framebuffer in time
    cmdts[VDP1_CMDT_ORDER_ERASE_INDEX].cmd_xa = 0;
    cmdts[VDP1_CMDT_ORDER_ERASE_INDEX].cmd_ya = ((_render_height - 18) / 2) - 1;
    cmdts[VDP1_CMDT_ORDER_ERASE_INDEX].cmd_xb = 0;
    cmdts[VDP1_CMDT_ORDER_ERASE_INDEX].cmd_yb = 0;
    cmdts[VDP1_CMDT_ORDER_ERASE_INDEX].cmd_xc = _render_width - 1;
    cmdts[VDP1_CMDT_ORDER_ERASE_INDEX].cmd_yc = _render_height - 1;
    cmdts[VDP1_CMDT_ORDER_ERASE_INDEX].cmd_xd = 0;
    cmdts[VDP1_CMDT_ORDER_ERASE_INDEX].cmd_yd = 0;

    for (uint32_t i = VDP1_CMDT_ORDER_BUFFER_STARTING_INDEX; i < VDP1_CMDT_ORDER_BUFFER_END_INDEX; i++) {
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

static void _draw_init(void) {
    _scene.cmdt_lists[0] = vdp1_cmdt_list_alloc(VDP1_CMDT_ORDER_COUNT);
    assert(_scene.cmdt_lists[0] != nullptr);

    _scene.cmdt_lists[1] = vdp1_cmdt_list_alloc(VDP1_CMDT_ORDER_COUNT);
    assert(_scene.cmdt_lists[1] != nullptr);

    _draw_cmdt_list_init(_scene.cmdt_lists[0]);
    _draw_cmdt_list_init(_scene.cmdt_lists[1]);

    vdp1_cmdt_end_set(&_scene.cmdt_lists[0]->cmdts[VDP1_CMDT_ORDER_BUFFER_STARTING_INDEX]);
    vdp1_cmdt_end_set(&_scene.cmdt_lists[1]->cmdts[VDP1_CMDT_ORDER_BUFFER_STARTING_INDEX]);

    _scene.cmdt_lists[0]->count = VDP1_CMDT_ORDER_BUFFER_STARTING_INDEX + 1;
    _scene.cmdt_lists[1]->count = VDP1_CMDT_ORDER_BUFFER_STARTING_INDEX + 1;

    _scene.which_cmdt_list = 0;
    _scene.cmdt_list = _scene.cmdt_lists[_scene.which_cmdt_list];
}

static void _vblank_out_handler(void *) {
    _stats.vblank_count++;

    smpc_peripheral_intback_issue();
}

static void _frt_ovi_handler(void) {
    _stats.ovi_count++;
}

static void _on_start(uint32_t, bool) {
    // At the beginning of a processing frame, clear the previous Draw End
    // command
    vdp1_cmdt* const end_cmdt =
            &_scene.cmdt_list->cmdts[_scene.cmdt_list->count - 1];

    end_cmdt->cmd_ctrl &= ~0x8000;

    _scene.cmdt_list->count = VDP1_CMDT_ORDER_BUFFER_STARTING_INDEX;
}

static void _on_end(uint32_t frame_index __unused, bool last_frame) {
    if (!last_frame) {
        vdp1_cmdt* const end_cmdt = &_scene.cmdt_list->cmdts[_scene.cmdt_list->count];

        vdp1_cmdt_end_set(end_cmdt);

        _scene.cmdt_list->count++;

        /* Wait for the previous sync (if any) */
        vdp1_sync_wait();
        vdp1_sync_cmdt_list_put(_scene.cmdt_list, 0, NULL, NULL);

        /* Call to sync -- does not block */
        vdp1_sync();

        /* Switch over to begin populating the other command table list */
        _scene.which_cmdt_list ^= 1;
        _scene.cmdt_list = _scene.cmdt_lists[_scene.which_cmdt_list];
    } else {
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

    _scene.palette[palette_index] = rgb1555_color;

    MEMORY_WRITE(16, VDP2_CRAM((palette_index + 16) << 1), rgb1555_color.raw);
}

static void _on_clear_screen(bool clear_screen) {
    if (!clear_screen) {
        vdp1_sync_mode_set(VDP1_SYNC_MODE_CHANGE_ONLY);
    } else {
        vdp1_sync_mode_set(VDP1_SYNC_MODE_ERASE_CHANGE);
    }
}

static void _triangle_vertex_set(vdp1_cmdt& cmdt,
                                 const uint8_vec2_t* vertices) {
    uint8_t* p = (uint8_t*)&cmdt.cmd_xa;

    p++;

    // A
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // B
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // C
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices -= 2;

    // D
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
}

static void _quad_vertex_set(vdp1_cmdt& cmdt,
                             const uint8_vec2_t* vertices) {
    uint8_t* p = (uint8_t*)&cmdt.cmd_xa;

    p++;

    // A
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // B
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // C
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // D
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
}

static void _triangle_vertex_set(vdp1_cmdt& cmdt,
                                 const uint8_vec2_t* vertices,
                                 uint32_t start_index) {
    uint8_t* p = (uint8_t*)&cmdt.cmd_xa;

    p++;

    // A
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    const uint8_vec2_t* next_vertices = vertices + start_index;

    // B
    *p = next_vertices->x;
    p += 2;
    *p = next_vertices->y;
    p += 2;

    next_vertices++;

    // C
    *p = next_vertices->x;
    p += 2;
    *p = next_vertices->y;
    p += 2;

    // D
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
}

static void _quad_vertex_set(vdp1_cmdt& cmdt,
                             const uint8_vec2_t* vertices,
                             uint32_t start_index) {
    uint8_t* p = (uint8_t*)&cmdt.cmd_xa;

    p++;

    // A
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices += start_index;

    // B
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // C
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
    p += 2;

    vertices++;

    // D
    *p = vertices->x;
    p += 2;
    *p = vertices->y;
}

static uint32_t _on_draw_polygon3(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  vdp1_cmdt_color_bank_t color_bank) {
    cmdt[0].cmd_colr = color_bank.raw;
    _triangle_vertex_set(cmdt[0], vertex_buffer);

    return 1;
}

static uint32_t _on_draw_polygon4(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  vdp1_cmdt_color_bank_t color_bank) {
    cmdt[0].cmd_colr = color_bank.raw;
    _quad_vertex_set(cmdt[0], vertex_buffer);

    return 1;
}

static uint32_t _on_draw_polygon5(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  vdp1_cmdt_color_bank_t color_bank) {
    cmdt[0].cmd_colr = color_bank.raw;
    _quad_vertex_set(cmdt[0], vertex_buffer);

    cmdt[1].cmd_colr = color_bank.raw;
    _triangle_vertex_set(cmdt[1], vertex_buffer, 3);

    return 2;
}

static uint32_t _on_draw_polygon6(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  vdp1_cmdt_color_bank_t color_bank) {
    cmdt[0].cmd_colr = color_bank.raw;
    _quad_vertex_set(cmdt[0], vertex_buffer);

    cmdt[1].cmd_colr = color_bank.raw;
    _quad_vertex_set(cmdt[1], vertex_buffer, 3);

    return 2;
}

static uint32_t _on_draw_polygon7(vdp1_cmdt* cmdt,
                                  const uint8_vec2_t* vertex_buffer,
                                  vdp1_cmdt_color_bank_t color_bank) {
    cmdt[0].cmd_colr = color_bank.raw;
    _quad_vertex_set(cmdt[0], vertex_buffer);

    cmdt[1].cmd_colr = color_bank.raw;
    _quad_vertex_set(cmdt[1], vertex_buffer, 3);

    cmdt[2].cmd_colr = color_bank.raw;
    _triangle_vertex_set(cmdt[2], vertex_buffer, 5);

    return 3;
}

static void _on_draw(const uint8_vec2_t* vertex_buffer,
                     uint32_t count,
                     uint32_t palette_index) {
    const draw_handler draw_handler = _draw_handlers[count];

    vdp1_cmdt* cmdt = &_scene.cmdt_list->cmdts[_scene.cmdt_list->count];

    vdp1_cmdt_color_bank_t color_bank;
    color_bank.raw = 0x0000;
    color_bank.type_0.data.dc = palette_index + 16;

    _scene.cmdt_list->count += draw_handler(cmdt, vertex_buffer, color_bank);
}

// #pragma GCC pop_options
