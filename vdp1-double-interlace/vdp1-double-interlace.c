/*
 * Copyright (c) 2012-2017 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

#define ORDER_SYSTEM_CLIP_COORDS_INDEX  0
#define ORDER_LOCAL_COORDS_INDEX        1
#define ORDER_POLYGON_INDEX             2
#define ORDER_DRAW_END_INDEX            3
#define ORDER_COUNT                     4

static void _hardware_init(void);

static void _vblank_out_handler(void *);

static void _vdp1_drawing_list_init(vdp1_cmdt_list_t *);
static void _vdp1_drawing_list_set(const uint8_t, vdp1_cmdt_list_t *);
static void _vdp1_drawing_env_toggle(const uint8_t);
static void _vdp2_resolution_toggle(const uint8_t);

static color_rgb1555_t _palette[16] = {
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),

        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),

        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),

        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0),
        COLOR_RGB1555_INITIALIZER(1, 0, 0, 0)
};

void
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        uint8_t switch_env;
        switch_env = 0;
        bool switched;
        switched = true;

        fix16_t speed;
        speed = FIX16(0.0f);

        smpc_peripheral_digital_t digital;

        vdp1_cmdt_list_t *cmdt_list;
        cmdt_list = vdp1_cmdt_list_alloc(5);

        _vdp1_drawing_list_init(cmdt_list);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &digital);

                if ((digital.held.button.a) != 0) {
                        switch_env ^= 1;

                        switched = true;
                }

                if ((digital.held.button.start) != 0) {
                        smpc_smc_sysres_call();
                }

                uint32_t speed_int;
                speed_int = fix16_int32_to(speed);

                volatile color_rgb1555_t *palette_ptr;
                palette_ptr = &_palette[0];

                palette_ptr->r = speed_int;
                palette_ptr->g = speed_int;
                palette_ptr->b = speed_int;

                MEMORY_WRITE(16, VDP2_CRAM(0x20), _palette[0].raw);

                char buffer[64];
                dbgio_puts("[H[2Jspeed: ");
                fix16_str(speed, buffer, 7);
                dbgio_puts(buffer);
                dbgio_puts("\n");

                speed = (speed >= FIX16(31.0f)) ? FIX16(0.0f) : (speed + FIX16(0.5f));

                if (switched) {
                        _vdp1_drawing_env_toggle(switch_env);
                        _vdp2_resolution_toggle(switch_env);

                        switched = false;
                }

                _vdp1_drawing_list_set(switch_env, cmdt_list);

                dbgio_flush();

                vdp1_sync_cmdt_list_put(cmdt_list, 0, NULL, NULL);
                vdp_sync();
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_DOUBLE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 0, 0));

        vdp2_sprite_priority_set(0, 6);
        vdp2_sprite_priority_set(1, 6);
        vdp2_sprite_priority_set(2, 6);
        vdp2_sprite_priority_set(3, 6);
        vdp2_sprite_priority_set(4, 6);
        vdp2_sprite_priority_set(5, 6);
        vdp2_sprite_priority_set(6, 6);
        vdp2_sprite_priority_set(7, 6);

        vdp_sync_vblank_out_set(_vblank_out_handler);

        vdp2_tvmd_display_set();

        cpu_intc_mask_set(0);
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_vdp1_drawing_list_init(vdp1_cmdt_list_t *cmdt_list)
{
        static const int16_vec2_t local_coord_ul =
            INT16_VEC2_INITIALIZER(0,
                                      0);

        static const vdp1_cmdt_draw_mode_t polygon_draw_mode = {
                .raw = 0x0000,
                .bits.pre_clipping_disable = true
        };

        assert(cmdt_list != NULL);

        vdp1_cmdt_t *cmdts;
        cmdts = &cmdt_list->cmdts[0];

        (void)memset(&cmdts[0], 0x00, sizeof(vdp1_cmdt_t) * ORDER_COUNT);

        cmdt_list->count = ORDER_COUNT;

        vdp1_cmdt_polygon_set(&cmdts[ORDER_POLYGON_INDEX]);
        vdp1_cmdt_param_draw_mode_set(&cmdts[ORDER_POLYGON_INDEX], polygon_draw_mode);

        vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);

        vdp1_cmdt_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_LOCAL_COORDS_INDEX],
            CMDT_VTX_LOCAL_COORD, &local_coord_ul);

        vdp1_cmdt_end_set(&cmdts[ORDER_DRAW_END_INDEX]);
}

static void
_vdp1_drawing_list_set(const uint8_t switch_env, vdp1_cmdt_list_t *cmdt_list)
{
        static vdp1_cmdt_color_bank_t polygon_color_bank = {
                .type_0.data.dc = 16
        };

        assert(cmdt_list != NULL);

        vdp1_cmdt_t *cmdts;
        cmdts = &cmdt_list->cmdts[0];

        vdp1_cmdt_t *cmdt_polygon;
        cmdt_polygon = &cmdts[ORDER_POLYGON_INDEX];

        vdp1_cmdt_t *cmdt_system_clip_coords;
        cmdt_system_clip_coords = &cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX];

        switch (switch_env & 0x01) {
        case 0:
                cmdt_system_clip_coords->cmd_xc = SCREEN_WIDTH - 1;
                cmdt_system_clip_coords->cmd_yc = SCREEN_HEIGHT - 1;

                polygon_color_bank.type_8.data.dc = 16;

                cmdt_polygon->cmd_xa = 0;
                cmdt_polygon->cmd_ya = SCREEN_HEIGHT - 1;

                cmdt_polygon->cmd_xb = SCREEN_WIDTH - 1;
                cmdt_polygon->cmd_yb = SCREEN_HEIGHT - 1;

                cmdt_polygon->cmd_xc = SCREEN_WIDTH - 1;
                cmdt_polygon->cmd_yc = 0;

                cmdt_polygon->cmd_xd = 0;
                cmdt_polygon->cmd_yd = 0;
                break;
        case 1:
                cmdt_system_clip_coords->cmd_xc = (SCREEN_WIDTH / 2) - 1;
                cmdt_system_clip_coords->cmd_yc = (SCREEN_HEIGHT / 2) - 1;

                polygon_color_bank.type_0.data.dc = 16;

                cmdt_polygon->cmd_xa = 0;
                cmdt_polygon->cmd_ya = (SCREEN_HEIGHT / 2) - 1;

                cmdt_polygon->cmd_xb = (SCREEN_WIDTH / 2) - 1;
                cmdt_polygon->cmd_yb = (SCREEN_HEIGHT / 2) - 1;

                cmdt_polygon->cmd_xc = (SCREEN_WIDTH / 2) - 1;
                cmdt_polygon->cmd_yc = 0;

                cmdt_polygon->cmd_xd = 0;
                cmdt_polygon->cmd_yd = 0;
                break;
        }

        vdp1_cmdt_param_color_bank_set(cmdt_polygon, polygon_color_bank);
}

static void
_vdp1_drawing_env_toggle(const uint8_t switch_env)
{
        static vdp1_env_t vdp1_env = {
                .erase_color = COLOR_RGB1555(1, 0, 0, 0),
                .erase_points[0] = {
                        .x = 0,
                        .y = 0
                },
                .erase_points[1] = {
                        .x = SCREEN_WIDTH - 1,
                        .y = SCREEN_HEIGHT -1
                },
                .bpp = VDP1_ENV_BPP_8,
                .rotation = VDP1_ENV_ROTATION_0,
                .color_mode = VDP1_ENV_COLOR_MODE_PALETTE,
                .sprite_type = 0x8
        };

        switch (switch_env & 0x01) {
        case 0:
                vdp1_env.bpp = VDP1_ENV_BPP_8;
                vdp1_env.sprite_type = 0x8;
                vdp1_env.color_mode = VDP1_ENV_COLOR_MODE_PALETTE;

                vdp1_env.erase_points[1].x = SCREEN_WIDTH - 1;
                vdp1_env.erase_points[1].y = SCREEN_HEIGHT - 1;
                break;
        case 1:
                vdp1_env.bpp = VDP1_ENV_BPP_16;
                vdp1_env.sprite_type = 0x0;
                vdp1_env.color_mode = VDP1_ENV_COLOR_MODE_RGB_PALETTE;

                vdp1_env.erase_points[1].x = (SCREEN_WIDTH / 2) - 1;
                vdp1_env.erase_points[1].y = (SCREEN_HEIGHT / 2) - 1;
                break;
        }

        vdp1_env_set(&vdp1_env);
}

static void
_vdp2_resolution_toggle(const uint8_t switch_env)
{
        switch (switch_env) {
        case 0:
                vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_DOUBLE, VDP2_TVMD_HORZ_NORMAL_A,
                    VDP2_TVMD_VERT_240);
                break;
        case 1:
                vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                    VDP2_TVMD_VERT_240);
                break;
        }
}
