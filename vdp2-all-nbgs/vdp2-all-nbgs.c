/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdbool.h>

#define NBGX_CPD         VDP2_VRAM_ADDR(0, 0x04000)
#define NBGX_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)

#define NBG0_MAP         NBG0_MAP_PLANE_A
#define NBG0_MAP_PLANE_A VDP2_VRAM_ADDR(1, 0x00000)
#define NBG0_MAP_PLANE_B VDP2_VRAM_ADDR(1, 0x02000)
#define NBG0_MAP_PLANE_C VDP2_VRAM_ADDR(1, 0x02000)
#define NBG0_MAP_PLANE_D VDP2_VRAM_ADDR(1, 0x02000)

#define NBG1_MAP         NBG1_MAP_PLANE_B
#define NBG1_MAP_PLANE_A VDP2_VRAM_ADDR(1, 0x02000)
#define NBG1_MAP_PLANE_B VDP2_VRAM_ADDR(1, 0x00800)
#define NBG1_MAP_PLANE_C VDP2_VRAM_ADDR(1, 0x02000)
#define NBG1_MAP_PLANE_D VDP2_VRAM_ADDR(1, 0x02000)

#define NBG2_MAP         NBG2_MAP_PLANE_C
#define NBG2_MAP_PLANE_A VDP2_VRAM_ADDR(1, 0x02000)
#define NBG2_MAP_PLANE_B VDP2_VRAM_ADDR(1, 0x02000)
#define NBG2_MAP_PLANE_C VDP2_VRAM_ADDR(1, 0x01000)
#define NBG2_MAP_PLANE_D VDP2_VRAM_ADDR(1, 0x02000)

#define NBG3_MAP         NBG3_MAP_PLANE_D
#define NBG3_MAP_PLANE_A VDP2_VRAM_ADDR(1, 0x02000)
#define NBG3_MAP_PLANE_B VDP2_VRAM_ADDR(1, 0x02000)
#define NBG3_MAP_PLANE_C VDP2_VRAM_ADDR(1, 0x02000)
#define NBG3_MAP_PLANE_D VDP2_VRAM_ADDR(1, 0x01800)

#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x01FFFE)

extern uint8_t asset_cpd[];
extern uint8_t asset_cpd_end[];

extern uint8_t asset_nbg0_pnd[];
extern uint8_t asset_nbg0_pnd_end[];

extern uint8_t asset_nbg1_pnd[];
extern uint8_t asset_nbg1_pnd_end[];

extern uint8_t asset_nbg2_pnd[];
extern uint8_t asset_nbg2_pnd_end[];

extern uint8_t asset_nbg3_pnd[];
extern uint8_t asset_nbg3_pnd_end[];

extern uint8_t asset_pal[];
extern uint8_t asset_pal_end[];

static void _vblank_out_handler(void *work);

static smpc_peripheral_digital_t _digital;

int
main(void)
{
        vdp2_scrn_cell_format_t nbgx_format = {
                .ccc           = VDP2_SCRN_CCC_PALETTE_256,
                .char_size     = VDP2_SCRN_CHAR_SIZE_2X2,
                .pnd_size      = 1,
                .aux_mode      = VDP2_SCRN_AUX_MODE_1,
                .plane_size    = VDP2_SCRN_PLANE_SIZE_1X1,
                .cpd_base      = NBGX_CPD,
                .palette_base  = NBGX_PAL
        };

        const vdp2_scrn_normal_map_t nbg0_normal_map = {
                .plane_a = NBG0_MAP_PLANE_A,
                .plane_b = NBG0_MAP_PLANE_B,
                .plane_c = NBG0_MAP_PLANE_C,
                .plane_d = NBG0_MAP_PLANE_D
        };

        const vdp2_scrn_normal_map_t nbg1_normal_map = {
                .plane_a = NBG1_MAP_PLANE_A,
                .plane_b = NBG1_MAP_PLANE_B,
                .plane_c = NBG1_MAP_PLANE_C,
                .plane_d = NBG1_MAP_PLANE_D
        };

        const vdp2_scrn_normal_map_t nbg2_normal_map = {
                .plane_a = NBG2_MAP_PLANE_A,
                .plane_b = NBG2_MAP_PLANE_B,
                .plane_c = NBG2_MAP_PLANE_C,
                .plane_d = NBG2_MAP_PLANE_D
        };

        const vdp2_scrn_normal_map_t nbg3_normal_map = {
                .plane_a = NBG3_MAP_PLANE_A,
                .plane_b = NBG3_MAP_PLANE_B,
                .plane_c = NBG3_MAP_PLANE_C,
                .plane_d = NBG3_MAP_PLANE_D
        };

        const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG1,
                .pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG1,
                .pt[0].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG2,
                .pt[0].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG2,
                .pt[0].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG3,
                .pt[0].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG3,

                .pt[1].t0 = VDP2_VRAM_CYCP_PNDR_NBG0,
                .pt[1].t1 = VDP2_VRAM_CYCP_PNDR_NBG0,
                .pt[1].t2 = VDP2_VRAM_CYCP_PNDR_NBG1,
                .pt[1].t3 = VDP2_VRAM_CYCP_PNDR_NBG1,
                .pt[1].t4 = VDP2_VRAM_CYCP_PNDR_NBG2,
                .pt[1].t5 = VDP2_VRAM_CYCP_PNDR_NBG2,
                .pt[1].t6 = VDP2_VRAM_CYCP_PNDR_NBG3,
                .pt[1].t7 = VDP2_VRAM_CYCP_PNDR_NBG3,

                .pt[2].t0 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[3].t0 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS
        };

        vdp2_vram_cycp_set(&vram_cycp);

        scu_dma_transfer(0, (void *)NBGX_PAL, asset_pal, asset_pal_end - asset_pal);
        scu_dma_transfer(0, (void *)NBG0_MAP, asset_nbg0_pnd, asset_nbg0_pnd_end - asset_nbg0_pnd);
        scu_dma_transfer(0, (void *)NBG1_MAP, asset_nbg1_pnd, asset_nbg1_pnd_end - asset_nbg1_pnd);
        scu_dma_transfer(0, (void *)NBG2_MAP, asset_nbg2_pnd, asset_nbg2_pnd_end - asset_nbg2_pnd);
        scu_dma_transfer(0, (void *)NBG3_MAP, asset_nbg3_pnd, asset_nbg3_pnd_end - asset_nbg3_pnd);
        scu_dma_transfer(0, (void *)NBGX_CPD, asset_cpd, asset_cpd_end - asset_cpd);

        nbgx_format.scroll_screen = VDP2_SCRN_NBG0;
        vdp2_scrn_cell_format_set(&nbgx_format, &nbg0_normal_map);

        nbgx_format.scroll_screen = VDP2_SCRN_NBG1;
        vdp2_scrn_cell_format_set(&nbgx_format, &nbg1_normal_map);

        nbgx_format.scroll_screen = VDP2_SCRN_NBG2;
        vdp2_scrn_cell_format_set(&nbgx_format, &nbg2_normal_map);

        nbgx_format.scroll_screen = VDP2_SCRN_NBG3;
        vdp2_scrn_cell_format_set(&nbgx_format, &nbg3_normal_map);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG1, 7);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG2, 7);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG3, 7);

        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0, FIX16(0.0f));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0, FIX16(0.0f));

        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG1, FIX16(0.0f));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG1, FIX16(0.0f));

        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG2, FIX16(0.0f));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG2, FIX16(0.0f));

        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG3, FIX16(0.0f));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG3, FIX16(0.0f));

        vdp2_scrn_disp_t disp_mask;
        disp_mask = VDP2_SCRN_DISPTP_NBG0 |
                    VDP2_SCRN_DISPTP_NBG1 |
                    VDP2_SCRN_DISPTP_NBG2 |
                    VDP2_SCRN_DISPTP_NBG3;

        vdp2_scrn_display_set(disp_mask);

        vdp2_sync();
        vdp2_sync_wait();

        vdp2_tvmd_display_set();

        fix16_vec2_t pos = FIX16_VEC2_INITIALIZER(0.0f, 0.0f);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                if (_digital.held.button.start) {
                        pos.x = FIX16(0.0f);
                        pos.y = FIX16(0.0f);
                } else {
                        const vdp2_scrn_disp_t old_disp_mask = disp_mask;

                        if (_digital.held.button.x) {
                                disp_mask ^= VDP2_SCRN_DISP_NBG0;
                        }
                        if (_digital.held.button.y) {
                                disp_mask ^= VDP2_SCRN_DISP_NBG1;
                        }
                        if (_digital.held.button.a) {
                                disp_mask ^= VDP2_SCRN_DISP_NBG2;
                        }
                        if (_digital.held.button.b) {
                                disp_mask ^= VDP2_SCRN_DISP_NBG3;
                        }

                        if (old_disp_mask != disp_mask) {
                                vdp2_scrn_display_set(disp_mask);
                        }

                        if (_digital.pressed.button.up) {
                                pos.y += FIX16(-0.5f);
                        } else if (_digital.pressed.button.down) {
                                pos.y += FIX16( 0.5f);
                        }

                        if (_digital.pressed.button.left) {
                                pos.x += FIX16(-0.5f);
                        } else if (_digital.pressed.button.right) {
                                pos.x += FIX16( 0.5f);
                        }

                        pos.x = clamp(pos.x, FIX16(-4.0f), FIX16(4.0f));
                        pos.y = clamp(pos.y, FIX16(-4.0f), FIX16(4.0f));
                }

                const fix16_vec2_t clamped_pos = {
                        .x = pos.x & 0xFFFF0000,
                        .y = pos.y & 0xFFFF0000
                };

                vdp2_scrn_scroll_x_update(VDP2_SCRN_NBG0, clamped_pos.x);
                vdp2_scrn_scroll_x_update(VDP2_SCRN_NBG1, clamped_pos.x);
                vdp2_scrn_scroll_x_update(VDP2_SCRN_NBG2, clamped_pos.x);
                vdp2_scrn_scroll_x_update(VDP2_SCRN_NBG3, clamped_pos.x);

                vdp2_scrn_scroll_y_update(VDP2_SCRN_NBG0, clamped_pos.y);
                vdp2_scrn_scroll_y_update(VDP2_SCRN_NBG1, clamped_pos.y);
                vdp2_scrn_scroll_y_update(VDP2_SCRN_NBG2, clamped_pos.y);
                vdp2_scrn_scroll_y_update(VDP2_SCRN_NBG3, clamped_pos.y);

                vdp2_sync();
                vdp2_sync_wait();
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        smpc_peripheral_init();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}
