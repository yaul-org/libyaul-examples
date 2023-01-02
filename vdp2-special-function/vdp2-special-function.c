/*
 * Copyright (c) 2012-2018 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <sys/queue.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

#define VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX 0
#define VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX       1
#define VDP1_CMDT_ORDER_POLYGON_INDEX            2
#define VDP1_CMDT_ORDER_DRAW_END_INDEX           3
#define VDP1_CMDT_ORDER_COUNT                    (VDP1_CMDT_ORDER_DRAW_END_INDEX + 1)

#define NBG0_CPD         VDP2_VRAM_ADDR(0, 0x00000)
#define NBG0_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define NBG0_MAP_PLANE_A VDP2_VRAM_ADDR(3, 0x00000)
#define NBG0_MAP_PLANE_B VDP2_VRAM_ADDR(3, 0x00000)
#define NBG0_MAP_PLANE_C VDP2_VRAM_ADDR(3, 0x00000)
#define NBG0_MAP_PLANE_D VDP2_VRAM_ADDR(3, 0x00000)

#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x01FFFE)

extern uint8_t asset_cpd_dat[];
extern uint8_t asset_cpd_dat_end[];
extern uint8_t asset_pnd_dat[];
extern uint8_t asset_pnd_dat_end[];
extern uint8_t asset_pal_dat[];
extern uint8_t asset_pal_dat_end[];

static void _cmdt_list_init(vdp1_cmdt_list_t *cmdt_list);

void
main(void)
{
        scu_dma_transfer(0, (void *)NBG0_CPD, asset_cpd_dat, asset_cpd_dat_end - asset_cpd_dat);
        scu_dma_transfer(0, (void *)NBG0_MAP_PLANE_A, asset_pnd_dat, asset_pnd_dat_end - asset_pnd_dat);
        scu_dma_transfer(0, (void *)NBG0_PAL, asset_pal_dat, asset_pal_dat_end - asset_pal_dat);

        vdp2_tvmd_display_set();

        vdp1_cmdt_list_t * const cmdt_list =
            vdp1_cmdt_list_alloc(VDP1_CMDT_ORDER_COUNT);

        _cmdt_list_init(cmdt_list);

        vdp1_sync_cmdt_list_put(cmdt_list, 0);
        vdp1_sync_render();
        vdp1_sync();
        vdp1_sync_wait();

        bool show_polygon;
        show_polygon = false;

        uint32_t count_frames;
        count_frames = 0;

        while (true) {
                vdp1_cmdt_t * const cmdt_polygon = &cmdt_list->cmdts[VDP1_CMDT_ORDER_POLYGON_INDEX];

                if (show_polygon) {
                        vdp1_cmdt_jump_next(cmdt_polygon);
                } else {
                        vdp1_cmdt_jump_skip_next(cmdt_polygon);
                }

                vdp1_sync_cmdt_list_put(cmdt_list, 0);

                vdp1_sync_render();
                vdp1_sync();

                vdp2_sync();
                vdp1_sync_wait();
                vdp2_sync_wait();

                count_frames++;

                if (count_frames >= 15) {
                        count_frames = 0;
                        show_polygon ^= true;
                }
        }
}

void
user_init(void)
{
        const vdp2_scrn_cell_format_t format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .ccc           = VDP2_SCRN_CCC_PALETTE_2048,
                .char_size     = VDP2_SCRN_CHAR_SIZE_1X1,
                .pnd_size      = 2,
                .aux_mode      = VDP2_SCRN_AUX_MODE_1,
                .plane_size    = VDP2_SCRN_PLANE_SIZE_1X1,
                .cpd_base      = NBG0_CPD,
                .palette_base  = NBG0_PAL
        };

        const vdp2_scrn_normal_map_t normal_map = {
                .plane_a = NBG0_MAP_PLANE_A,
                .plane_b = NBG0_MAP_PLANE_B,
                .plane_c = NBG0_MAP_PLANE_C,
                .plane_d = NBG0_MAP_PLANE_D
        };

        vdp2_scrn_cell_format_set(&format, &normal_map);

        vdp2_scrn_sf_set(VDP2_SCRN_NBG0, VDP2_SCRN_SF_MODE_2,
            VDP2_SCRN_SF_TYPE_CELL_PRIORITY, VDP2_SCRN_SF_CODE_A);

        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 0, 0, 7));

        /* The special priority function only toggles the LSB, so the priority
         * must be even */
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 2);

        /* The lower two bits of the CPD are used to determine which pixel in
         * the character pattern data will be above or below the VDP1 layer.
         *
         * Considering that we need an extra two bits and still make use of
         * 256-colors (8-BPP), paletted 2048 (16-BPP) is the perfect choice for
         * this.
         *
         * We can still designate 8 of the 10 bits within the CPD.
         *
         * The downside is that the 256-color palette needs to be duplicated 4
         * times, resulting in using 1,024 colors in CRAM (exactly half in CRAM
         * mode 1). The reason being is that the special function makes use of
         * the lower 4-bits of the CPD.
         *
         * The codes (0x02, 0x06, 0x0A, and 0x0E) are used. The "odd" codes can
         * also be used: (0x03, 0x07, 0x0B, and 0x0F).
         */

        vdp2_scrn_sf_codes_set(VDP2_SCRN_SF_CODE_A,
                               VDP2_SCRN_SF_CODE_0x02_0x03 |
                               VDP2_SCRN_SF_CODE_0x06_0x07 |
                               VDP2_SCRN_SF_CODE_0x0A_0x0B |
                               VDP2_SCRN_SF_CODE_0x0E_0x0F);

        vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);

        /* Each cell is 128 bytes (8x8 cell, at 2-bytes per pixel). To fill a
         * 40x30 cell background, 0x25800 bytes is needed. A single VRAM bank
         * (set at 4-Mbit) is 0x20000 bytes. Without going across bank
         * boundaries, we need to not part a bank into two */
        const vdp2_vram_ctl_t vram_ctl = {
                .vram_mode = VDP2_VRAM_CTL_MODE_NO_PART_BANK_A | VDP2_VRAM_CTL_MODE_PART_BANK_B
        };

        vdp2_vram_control_set(&vram_ctl);

        vdp2_cram_mode_set(1);

        const vdp2_vram_cycp_bank_t vram_cycp_bank[2] = {
                {
                        .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                        .t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                        .t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                        .t7 = VDP2_VRAM_CYCP_NO_ACCESS
                }, {
                        .t0 = VDP2_VRAM_CYCP_PNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_PNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                        .t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                        .t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                        .t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                        .t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                        .t7 = VDP2_VRAM_CYCP_NO_ACCESS
                }
        };

        /* When bank A (or B) is not parted, cycle patterns for bank A1 does not
         * need to be set */
        vdp2_vram_cycp_bank_set(0, &vram_cycp_bank[0]);
        vdp2_vram_cycp_bank_clear(1);
        vdp2_vram_cycp_bank_clear(2);
        vdp2_vram_cycp_bank_set(3, &vram_cycp_bank[1]);

        const vdp1_env_t vdp1_env = {
                .erase_color  = RGB1555_INITIALIZER(0, 0, 0, 0),
                .erase_points = {
                        INT16_VEC2_INITIALIZER(0, 0),
                        INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1)
                },
                .bpp          = VDP1_ENV_BPP_16,
                .rotation     = VDP1_ENV_ROTATION_0,
                .color_mode   = VDP1_ENV_COLOR_MODE_RGB_PALETTE,
                .sprite_type  = 5
        };

        vdp1_env_set(&vdp1_env);

        /* Sprite type 5 has 3 bits dedicated to priority, allowing the
         * selection of 7 of the sprite registers */
        vdp2_sprite_priority_set(0, 2);
        vdp2_sprite_priority_set(1, 2);
        vdp2_sprite_priority_set(2, 2);
        vdp2_sprite_priority_set(3, 2);
        vdp2_sprite_priority_set(4, 2);
        vdp2_sprite_priority_set(5, 2);
        vdp2_sprite_priority_set(6, 2);
        vdp2_sprite_priority_set(7, 2);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_sync();
        vdp2_sync_wait();
}

static void
_cmdt_list_init(vdp1_cmdt_list_t *cmdt_list)
{
        const int16_vec2_t system_clip_coord =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                   SCREEN_HEIGHT - 1);

        const int16_vec2_t local_coord_ul =
            INT16_VEC2_INITIALIZER(0, 0);

        const vdp1_cmdt_draw_mode_t polygon_draw_mode = {
                .pre_clipping_disable = true
        };

        const int16_vec2_t polygon_points[] = {
                INT16_VEC2_INITIALIZER(0, SCREEN_HEIGHT - 1),
                INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1),
                INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,                 0),
                INT16_VEC2_INITIALIZER(               0,                 0)
        };

        vdp1_cmdt_t * const cmdts =
            &cmdt_list->cmdts[0];

        (void)memset(&cmdts[0], 0x00, sizeof(vdp1_cmdt_t) * VDP1_CMDT_ORDER_COUNT);

        cmdt_list->count = VDP1_CMDT_ORDER_COUNT;

        vdp1_cmdt_system_clip_coord_set(&cmdts[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_vtx_system_clip_coord_set(&cmdts[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX],
            system_clip_coord);

        vdp1_cmdt_local_coord_set(&cmdts[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_vtx_local_coord_set(&cmdts[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX],
            local_coord_ul);

        vdp1_cmdt_polygon_set(&cmdts[VDP1_CMDT_ORDER_POLYGON_INDEX]);
        vdp1_cmdt_draw_mode_set(&cmdts[VDP1_CMDT_ORDER_POLYGON_INDEX],
            polygon_draw_mode);
        vdp1_cmdt_color_set(&cmdts[VDP1_CMDT_ORDER_POLYGON_INDEX],
            RGB1555(1, 15, 15, 15));
        vdp1_cmdt_vtx_set(&cmdts[VDP1_CMDT_ORDER_POLYGON_INDEX],
            polygon_points);

        vdp1_cmdt_end_set(&cmdts[VDP1_CMDT_ORDER_DRAW_END_INDEX]);
}
