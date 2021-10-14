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

int
main(void)
{
        vdp2_scrn_bitmap_format_t format;
        memset(&format, 0x00, sizeof(format));

        format.scroll_screen = VDP2_SCRN_NBG0;
        format.cc_count = VDP2_SCRN_CCC_RGB_16770000;
        format.bitmap_size.width = 512;
        format.bitmap_size.height = 256;
        format.color_palette = 0x00000000;
        format.bitmap_pattern = VDP2_VRAM_ADDR(0, 0x00000);
        format.rp_mode = 0;
        format.sf_type = VDP2_SCRN_SF_TYPE_NONE;
        format.sf_code = VDP2_SCRN_SF_CODE_A;
        format.sf_mode = 0;

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* no_trans = */ false);

        vdp2_vram_cycp_t vram_cycp;

        vram_cycp.pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0;

        vram_cycp.pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0;

        vram_cycp.pt[2].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0;

        vram_cycp.pt[3].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0;

        vdp2_vram_cycp_set(&vram_cycp);

        uint32_t *vram;
        vram = (uint32_t *)VDP2_VRAM_ADDR(0, 0x00000);

        cpu_intc_mask_set (15); {
                uint32_t y;
                for (y = 0; y < 256; y++) {
                        uint32_t x;
                        for (x = 0; x < 512; x++) {
                                uint8_t r;
                                uint8_t g;
                                uint8_t b;

                                r = (x < 256) ? x : (255 - x);
                                g = y;
                                b = 0;

                                vram[x + (y * 512)] = ((b << 16) | (g << 8) | r) & 0x00FFFFFF;
                        }
                }
        } cpu_intc_mask_set(0);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_B,
            VDP2_TVMD_VERT_240);
        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }

        return 0;
}
