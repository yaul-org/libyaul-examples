/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdbool.h>

#define NBG0_CPD         VDP2_VRAM_ADDR(0, 0x00000)
#define NBG0_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define NBG0_MAP_PLANE_A VDP2_VRAM_ADDR(1, 0x00000)
#define NBG0_MAP_PLANE_B VDP2_VRAM_ADDR(1, 0x08000)
#define NBG0_MAP_PLANE_C VDP2_VRAM_ADDR(1, 0x10000)
#define NBG0_MAP_PLANE_D VDP2_VRAM_ADDR(1, 0x18000)

#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x01FFFE)

extern uint8_t asset_page_cpd[];
extern uint8_t asset_page_cpd_end[];
extern uint8_t asset_page_map[];
extern uint8_t asset_page_map_end[];
extern uint8_t asset_page_pal[];
extern uint8_t asset_page_pal_end[];

int
main(void)
{
        uint16_t * const cpd_base = (uint16_t *)NBG0_CPD;
        uint16_t * const palette_base = (uint16_t *)NBG0_PAL;

        const vdp2_scrn_cell_format_t format = {
                .scroll_screen = VDP2_SCRN_NBG1,
                .ccc           = VDP2_SCRN_CCC_PALETTE_256,
                .char_size     = VDP2_SCRN_CHAR_SIZE_1X1,
                .pnd_size      = 1,
                .aux_mode      = VDP2_SCRN_AUX_MODE_0,
                .plane_size    = VDP2_SCRN_PLANE_SIZE_2X2,
                .cpd_base      = NBG0_CPD,
                .palette_base  = NBG0_PAL,
        };

        const vdp2_scrn_normal_map_t normal_map = {
                .plane_a = NBG0_MAP_PLANE_A,
                .plane_b = NBG0_MAP_PLANE_B,
                .plane_c = NBG0_MAP_PLANE_C,
                .plane_d = NBG0_MAP_PLANE_D
        };

        const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG1,
                .pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG1,
                .pt[0].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[1].t0 = VDP2_VRAM_CYCP_PNDR_NBG1,
                .pt[1].t1 = VDP2_VRAM_CYCP_PNDR_NBG1,
                .pt[1].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

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

        scu_dma_transfer(0, palette_base, asset_page_pal, asset_page_pal_end - asset_page_pal);
        scu_dma_transfer(0, cpd_base, asset_page_cpd, asset_page_cpd_end - asset_page_cpd);

        const uint16_t page_size = VDP2_SCRN_CALCULATE_PAGE_SIZE(&format);

        /* Traverse through 4 planes, each of 2x2 pages */
        for (uint32_t j = 0; j < 4; j++) {
                vdp2_vram_t page = normal_map.base_addr[j];

                for (uint32_t i = 0; i < (2 * 2); i++) {
                        scu_dma_transfer(0, (void *)page, asset_page_map, asset_page_map_end - asset_page_map);

                        page += page_size;
                }
        }

        vdp2_scrn_cell_format_set(&format, &normal_map);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG1, 7);
        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG1, FIX16(0.0f));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG1, FIX16(0.0f));

        vdp2_scrn_display_set(VDP2_SCRN_NBG1_TPDISP);

        vdp2_sync();
        vdp2_sync_wait();

        vdp2_tvmd_display_set();

        while (true) {
                vdp2_scrn_scroll_x_update(VDP2_SCRN_NBG1, FIX16(4.0f));
                vdp2_scrn_scroll_y_update(VDP2_SCRN_NBG1, FIX16(4.0f));

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
}
