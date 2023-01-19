/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

int
main(void)
{
        const vdp2_scrn_bitmap_format_t format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .ccc           = VDP2_SCRN_CCC_RGB_16770000,
                .bitmap_size   = VDP2_SCRN_BITMAP_SIZE_512X256,
                .palette_base  = 0x00000000,
                .bitmap_base   = VDP2_VRAM_ADDR(0, 0x00000)
        };

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);

        const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0,

                .pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0,

                .pt[2].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0,

                .pt[3].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[3].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0
        };

        vdp2_vram_cycp_set(&vram_cycp);

        volatile rgb888_t * const vram =
            (volatile rgb888_t *)VDP2_VRAM_ADDR(0, 0x00000);

        for (uint32_t y = 0; y < 256; y++) {
                for (uint32_t x = 0; x < 512; x++) {
                        const uint8_t r = (x < 256) ? x : (255 - x);
                        const uint8_t g = y;
                        const uint8_t b = 0;

                        vram[x + (y * 512)] = RGB888(0, r, g, b);
                }
        }

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);
        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }

        return 0;
}
