/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <tga.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

extern uint8_t root_romdisk[];

int
main(void)
{
        vdp2_tvmd_display_clear();

        struct vdp2_scrn_bitmap_format format;
        memset(&format, 0x00, sizeof(format));

        format.sbf_scroll_screen = VDP2_SCRN_NBG0;
        format.sbf_cc_count = VDP2_SCRN_CCC_RGB_32768;
        format.sbf_bitmap_size.width = 512;
        format.sbf_bitmap_size.height = 256;
        format.sbf_color_palette = 0x00000000;
        format.sbf_bitmap_pattern = VDP2_VRAM_ADDR_4MBIT(0, 0x00000);
        format.sbf_rp_mode = 0;
        format.sbf_sf_type = VDP2_SCRN_SF_TYPE_NONE;
        format.sbf_sf_code = VDP2_SCRN_SF_CODE_A;
        format.sbf_sf_mode = 0;

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* no_trans = */ false);

        struct vdp2_vram_cycp vram_cycp;

        vram_cycp.pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[2].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[3].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_set(&vram_cycp);

        void *romdisk;
        void *fh; /* File handle */

        romdisk_init();

        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        fh = romdisk_open(romdisk, "/BITMAP.TGA");
        assert(fh != NULL);

        uint8_t *tga_file;
        tga_t tga;
        int ret;
        size_t len;

        tga_file = (uint8_t *)0x20200000;
        assert(tga_file != NULL);

        len = romdisk_read(fh, tga_file, romdisk_total(fh));
        assert(len == romdisk_total(fh));

        ret = tga_read(&tga, tga_file);
        assert(ret == TGA_FILE_OK);

        tga_image_decode(&tga, (void *)VDP2_VRAM_ADDR_4MBIT(0, 0x00000));

        color_rgb555_t bs_color;
        bs_color = COLOR_RGB555(5, 5, 7);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR_4MBIT(3, 0x01FFFE),
            bs_color);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);
        vdp2_tvmd_display_set();

        vdp_sync(0);

        while (true) {
        }

        return 0;
}
