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

extern uint8_t root_romdisk[];

static void vblank_in_handler(irq_mux_handle_t *);

int
main(void)
{
        void *romdisk;

        romdisk_init();

        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        vdp2_init();

        struct scrn_bitmap_format format;
        memset(&format, 0x00, sizeof(format));

        format.sbf_scroll_screen = SCRN_NBG0;
        format.sbf_cc_count = SCRN_CCC_PALETTE_256;
        format.sbf_bitmap_size.width = 1024;
        format.sbf_bitmap_size.height = 512;
        format.sbf_color_palette = CRAM_ADDR(0x300);
        format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(0, 0x00000);
        format.sbf_sf_type = SCRN_SF_TYPE_NONE;
        format.sbf_sf_code = SCRN_SF_CODE_A;
        format.sbf_sf_mode = 0;

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(SCRN_NBG0, 7);
        vdp2_scrn_reduction_set(SCRN_NBG0, SCRN_REDUCTION_HALF);
        vdp2_scrn_reduction_x_set(SCRN_NBG0, Q0_3_8(2.0f));
        vdp2_scrn_reduction_y_set(SCRN_NBG0, Q0_3_8(1.0f / (1.0f - (240.0f / 512.0f))));
        vdp2_scrn_display_set(SCRN_NBG0, /* no_trans = */ false);

        struct vram_ctl *vram_ctl;
        vram_ctl = vdp2_vram_control_get();

        vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[3].t0 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vdp2_vram_control_set(vram_ctl);

        void *fh;
        size_t len;

        fh = romdisk_open(romdisk, "/BITMAP_PAL.BIN");
        assert(fh != NULL);
        len = romdisk_read(fh, (void *)CRAM_ADDR(0x0000), romdisk_total(fh));
        assert(len == romdisk_total(fh));
        romdisk_close(fh);

        fh = romdisk_open(romdisk, "/BITMAP_CPD.BIN");
        assert(fh != NULL);
        len = romdisk_read(fh, (void *)VRAM_ADDR_4MBIT(0, 0x00000), romdisk_total(fh));
        assert(len == romdisk_total(fh));
        romdisk_close(fh);

        color_rgb555_t bs_color;
        bs_color = COLOR_RGB555(0x05, 0x05, 0x07);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE), bs_color);
        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A, TVMD_VERT_240);
        vdp2_tvmd_display_set();

        cpu_intc_mask_set (15); {
                irq_mux_t *vblank_in;
                vblank_in = vdp2_tvmd_vblank_in_irq_get();

                irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);
        } cpu_intc_mask_set(0);

        while (true) {
                vdp2_tvmd_vblank_out_wait();
                vdp2_tvmd_vblank_in_wait();
        }

        return 0;
}

static void
vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        vdp2_commit();
}
