/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdbool.h>

extern uint8_t asset_page_cpd[];
extern uint8_t asset_page_cpd_end[];
extern uint8_t asset_page_map[];
extern uint8_t asset_page_map_end[];
extern uint8_t asset_page_pal[];
extern uint8_t asset_page_pal_end[];

int
main(void)
{
        uint16_t * const color_palette =
            (uint16_t *)VDP2_CRAM_MODE_1_OFFSET(0, 0, 0);

        uint16_t * const planes[4] = {
                (uint16_t *)VDP2_VRAM_ADDR(1, 0x00000),
                (uint16_t *)VDP2_VRAM_ADDR(1, 0x08000),
                (uint16_t *)VDP2_VRAM_ADDR(1, 0x10000),
                (uint16_t *)VDP2_VRAM_ADDR(1, 0x18000)
        };

        uint16_t * const cpd = (uint16_t *)VDP2_VRAM_ADDR(0, 0x00000);

        const vdp2_scrn_cell_format_t format = {
                .scroll_screen     = VDP2_SCRN_NBG1,
                .cc_count          = VDP2_SCRN_CCC_PALETTE_256,
                .character_size    = 1 * 1,
                .pnd_size          = 1, /* 1-word */
                .auxiliary_mode    = 0,
                .plane_size        = 2 * 2,
                .cp_table          = (uint32_t)cpd,
                .color_palette     = (uint32_t)color_palette,
                .map_bases.plane_a = (uint32_t)planes[0],
                .map_bases.plane_b = (uint32_t)planes[1],
                .map_bases.plane_c = (uint32_t)planes[2],
                .map_bases.plane_d = (uint32_t)planes[3]
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

        scu_dma_transfer(0, color_palette, asset_page_pal, asset_page_pal_end - asset_page_pal);
        scu_dma_transfer(0, cpd, asset_page_cpd, asset_page_cpd_end - asset_page_cpd);

        const uint16_t page_size = VDP2_SCRN_CALCULATE_PAGE_SIZE(&format);

        /* Traverse through 4 planes, each of 2x2 pages */
        for (uint32_t j = 0; j < 4; j++) {
                uint32_t page;
                page = (uint32_t)planes[j];

                for (uint32_t i = 0; i < (2 * 2); i++) {
                        scu_dma_transfer(0, (void *)page, asset_page_map, asset_page_map_end - asset_page_map);

                        page += page_size;
                }
        }

        vdp2_scrn_cell_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG1, 7);
        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG1, FIX16(0.0f));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG1, FIX16(0.0f));

        vdp2_scrn_display_set(VDP2_SCRN_NBG1, /* transparent = */ true);

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

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));
}
