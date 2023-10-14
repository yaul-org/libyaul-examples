/*
 * Copyright (c) 2012-2017 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

#define ORDER_SYSTEM_CLIP_COORDS_INDEX 0
#define ORDER_LOCAL_COORDS_INDEX       1
#define ORDER_POLYGON_INDEX            2
#define ORDER_DRAW_END_INDEX           3
#define ORDER_COUNT                    4

static void _scu_timer_0_handler(void);

static void _vdp1_drawing_list_init(vdp1_cmdt_list_t *cmdt_list);
static void _vdp1_drawing_list_set(vdp1_cmdt_list_t *cmdt_list);
static void _vdp1_drawing_env_toggle(void);
static void _vdp2_resolution_toggle(void);

static volatile uint8_t _env_index           = 0;
static volatile bool _flag_change            = true;
static volatile bool _flag_resolution_change = false;
static volatile bool _flag_system_reset      = false;

int
main(void)
{
        fix16_t speed;
        speed = FIX16(0.0);

        vdp1_cmdt_list_t * const cmdt_list = vdp1_cmdt_list_alloc(5);

        _vdp1_drawing_list_init(cmdt_list);

        while (true) {
                if (_flag_resolution_change) {
                        _flag_resolution_change = false;

                        _vdp1_drawing_env_toggle();
                        _vdp1_drawing_list_set(cmdt_list);
                        vdp1_sync_wait();
                        vdp1_sync_cmdt_list_put(cmdt_list, 0);
                        vdp1_sync_render();
                        vdp1_sync();
                }

                smpc_peripheral_process();

                smpc_peripheral_digital_t digital;
                smpc_peripheral_digital_port(1, &digital);

                if ((digital.held.button.a) != 0) {
                        _env_index ^= 1;
                        _flag_change = true;
                }

                if ((digital.held.button.start) != 0) {
                        _flag_system_reset = true;

                        /* Wait here */
                        while (true) {
                        }
                }

                rgb1555_t color;

                const uint32_t speed_int = fix16_int32_to(speed);

                color.msb = 1;
                color.r = speed_int;
                color.g = speed_int;
                color.b = speed_int;

                MEMORY_WRITE(16, VDP2_CRAM(0x20), color.raw);

                const vdp1_env_t * const vdp1_env = vdp1_env_get();

                dbgio_printf("[6;3H[2J"
                             "Switch resolution (A) %ix%i\n"
                             "\n"
                             "  %f\n",
                             vdp1_env->erase_points[1].x + 1,
                             vdp1_env->erase_points[1].y + 1,
                             speed);

                speed = (speed >= FIX16(31.0)) ? FIX16(0.0) : (speed + FIX16(0.5));

                dbgio_flush();

                vdp2_sync();
                vdp2_sync_wait();
        }
}

void
user_init(void)
{
        smpc_peripheral_init();

        scu_timer_disable();
        /* Have SCU timer #0 trigger 6 scanlines before VBLANK-OUT. This is to
         * handle both NTSC and PAL */
        scu_timer_t0_value_set(256);
        scu_timer_t0_set(_scu_timer_0_handler);
        scu_timer_enable();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 0, 0));

        vdp2_sprite_priority_set(0, 6);
        vdp2_sprite_priority_set(1, 6);
        vdp2_sprite_priority_set(2, 6);
        vdp2_sprite_priority_set(3, 6);
        vdp2_sprite_priority_set(4, 6);
        vdp2_sprite_priority_set(5, 6);
        vdp2_sprite_priority_set(6, 6);
        vdp2_sprite_priority_set(7, 6);

        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        vdp2_tvmd_display_set();

        vdp2_sync();
}

static void
_scu_timer_0_handler(void)
{
        if (_flag_system_reset) {
                /* Issue 300µs after VBLANK-IN */
                smpc_smc_sysres_call();
        } else if (_flag_change) {
                /* Disable interrupts */
                const uint8_t sr_mask = cpu_intc_mask_get();
                cpu_intc_mask_set(15);
                scu_ic_mask_t mask = scu_ic_mask_get();
                scu_ic_mask_set(SCU_IC_MASK_ALL);

                /* Issue CKCHG320/CKCHG352 300µs after VBLANK-IN, but don't
                 * issue INTBACK until the next VBLANK-IN */
                _vdp2_resolution_toggle();

                /* Wait at least 100ms before issuing the next command. 7 is
                 * safe for either 50 or 60Hz */
                vdp2_tvmd_vblank_in_next_wait(7);

                _flag_change = false;
                _flag_resolution_change = true;

                /* Enable interrupts */
                scu_ic_mask_set(mask);
                cpu_intc_mask_set(sr_mask);
        } else {
                /* Issue 300µs after VBLANK-IN */
                smpc_peripheral_intback_issue();
        }
}

static void
_vdp1_drawing_list_init(vdp1_cmdt_list_t *cmdt_list)
{
        static const int16_vec2_t local_coord_ul =
            INT16_VEC2_INITIALIZER(0, 0);

        static const vdp1_cmdt_draw_mode_t polygon_draw_mode = {
                .pre_clipping_disable = true
        };

        assert(cmdt_list != NULL);

        vdp1_cmdt_t * const cmdts = &cmdt_list->cmdts[0];
        cmdt_list->count = ORDER_COUNT;

        (void)memset(&cmdts[0], 0x00, sizeof(vdp1_cmdt_t) * ORDER_COUNT);

        vdp1_cmdt_polygon_set(&cmdts[ORDER_POLYGON_INDEX]);
        vdp1_cmdt_draw_mode_set(&cmdts[ORDER_POLYGON_INDEX], polygon_draw_mode);

        vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);

        vdp1_cmdt_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_vtx_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX],
            local_coord_ul);

        vdp1_cmdt_end_set(&cmdts[ORDER_DRAW_END_INDEX]);
}

static void
_vdp1_drawing_list_set(vdp1_cmdt_list_t *cmdt_list)
{
        assert(cmdt_list != NULL);

        vdp1_cmdt_t * const cmdts = &cmdt_list->cmdts[0];

        vdp1_cmdt_t * const cmdt_polygon = &cmdts[ORDER_POLYGON_INDEX];
        vdp1_cmdt_t * const cmdt_system_clip_coord = &cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX];

        vdp1_cmdt_color_bank_t polygon_color_bank;
        polygon_color_bank.raw = 0x0000;

        int16_vec2_t system_clip_coord;

        switch (_env_index & 0x01) {
        case 0:
                system_clip_coord.x = SCREEN_WIDTH - 1;
                system_clip_coord.y = SCREEN_HEIGHT - 1;

                polygon_color_bank.type_8.dc = 16;

                cmdt_polygon->cmd_xa = 0;
                cmdt_polygon->cmd_ya = SCREEN_HEIGHT - 1;

                cmdt_polygon->cmd_xb = SCREEN_WIDTH - 1;
                cmdt_polygon->cmd_yb = SCREEN_HEIGHT - 1;

                cmdt_polygon->cmd_xc = SCREEN_WIDTH - 1;
                cmdt_polygon->cmd_yc = 0;

                cmdt_polygon->cmd_xd = 0;
                cmdt_polygon->cmd_yd = 0;

                vdp1_cmdt_vtx_system_clip_coord_set(cmdt_system_clip_coord,
                    system_clip_coord);
                break;
        case 1:
                system_clip_coord.x = (SCREEN_WIDTH / 2) - 1;
                system_clip_coord.y = (SCREEN_HEIGHT / 2) - 1;

                polygon_color_bank.type_0.dc = 16;

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

        vdp1_cmdt_vtx_system_clip_coord_set(cmdt_system_clip_coord,
            system_clip_coord);

        vdp1_cmdt_color_bank_set(cmdt_polygon, polygon_color_bank);
}

static void
_vdp1_drawing_env_toggle(void)
{
        static vdp1_env_t vdp1_env = {
                .erase_color     = RGB1555(1, 0, 0, 0),
                .erase_points[0] = INT16_VEC2_INITIALIZER(0, 0)
        };

        switch (_env_index & 0x01) {
        case 0:
                vdp1_env.bpp         = VDP1_ENV_BPP_8;
                vdp1_env.sprite_type = 8;
                vdp1_env.color_mode  = VDP1_ENV_COLOR_MODE_PALETTE;

                vdp1_env.erase_points[1].x = SCREEN_WIDTH - 1;
                vdp1_env.erase_points[1].y = SCREEN_HEIGHT - 1;
                break;
        case 1:
                vdp1_env.bpp         = VDP1_ENV_BPP_16;
                vdp1_env.sprite_type = 0;
                vdp1_env.color_mode  = VDP1_ENV_COLOR_MODE_RGB_PALETTE;

                vdp1_env.erase_points[1].x = (SCREEN_WIDTH / 2) - 1;
                vdp1_env.erase_points[1].y = (SCREEN_HEIGHT / 2) - 1;
                break;
        }

        vdp1_env_set(&vdp1_env);
}

static void
_vdp2_resolution_toggle(void)
{
        switch (_env_index) {
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
