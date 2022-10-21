/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

extern uint8_t asset_bitmap_cpd[];
extern uint8_t asset_bitmap_cpd_end[];
extern uint8_t asset_bitmap_pal[];
extern uint8_t asset_bitmap_pal_end[];

int
main(void)
{
        const vdp2_scrn_bitmap_format_t format = {
                .scroll_screen      = VDP2_SCRN_NBG0,
                .ccc                = VDP2_SCRN_CCC_PALETTE_256,
                .bitmap_size        = VDP2_SCRN_BITMAP_SIZE_1024X512,
                .palette_base       = VDP2_CRAM_ADDR(0x0300),
                .bitmap_base        = VDP2_VRAM_ADDR(0, 0x00000)
        };

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_reduction_set(VDP2_SCRN_NBG0, VDP2_SCRN_REDUCTION_HALF);
        vdp2_scrn_reduction_x_set(VDP2_SCRN_NBG0, FIX16(2.0f));
        vdp2_scrn_reduction_y_set(VDP2_SCRN_NBG0, FIX16(1.0f / (1.0f - (240.0f / 512.0f))));
        vdp2_scrn_display_set(VDP2_SCRN_NBG0_DISP);

        const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[2].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[3].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS
        };

        vdp2_vram_cycp_set(&vram_cycp);

        /* Set for CRAM mode 1: RGB 555 2,048 colors */
        vdp2_cram_mode_set(1);

        scu_dma_transfer(0, (void *)VDP2_VRAM_ADDR(0, 0x00000), asset_bitmap_cpd, asset_bitmap_cpd_end - asset_bitmap_cpd);
        scu_dma_transfer(0, (void *)VDP2_CRAM_ADDR(0x0000), asset_bitmap_pal, asset_bitmap_pal_end - asset_bitmap_pal);

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }

        return 0;
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));
}
