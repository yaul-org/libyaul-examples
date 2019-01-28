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
        struct scrn_bitmap_format format;
        memset(&format, 0x00, sizeof(format));

        format.sbf_scroll_screen = SCRN_NBG0;
        format.sbf_cc_count = SCRN_CCC_RGB_16770000;
        format.sbf_bitmap_size.width = 512;
        format.sbf_bitmap_size.height = 256;
        format.sbf_color_palette = 0x00000000;
        format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(0, 0x00000);
        format.sbf_rp_mode = 0;
        format.sbf_sf_type = SCRN_SF_TYPE_NONE;
        format.sbf_sf_code = SCRN_SF_CODE_A;
        format.sbf_sf_mode = 0;

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(SCRN_NBG0, 7);
        vdp2_scrn_display_set(SCRN_NBG0, /* no_trans = */ false);

        struct vram_cycp vram_cycp;

        vram_cycp.pt[0].t0 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t1 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t2 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t3 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t4 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t5 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t6 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t7 = VRAM_CYCP_CHPNDR_NBG0;

        vram_cycp.pt[1].t0 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t1 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t2 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t3 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t4 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t5 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t6 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t7 = VRAM_CYCP_CHPNDR_NBG0;

        vram_cycp.pt[2].t0 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t1 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t2 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t3 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t4 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t5 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t6 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t7 = VRAM_CYCP_CHPNDR_NBG0;

        vram_cycp.pt[3].t0 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t1 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t2 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t3 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t4 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t5 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t6 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t7 = VRAM_CYCP_CHPNDR_NBG0;

        vdp2_vram_cycp_set(&vram_cycp);

        uint32_t *vram;
        vram = (uint32_t *)VRAM_ADDR_4MBIT(0, 0x00000);

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

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_B,
            TVMD_VERT_240);
        vdp2_tvmd_display_set();

        vdp_sync(0);

        while (true) {
        }

        return 0;
}
