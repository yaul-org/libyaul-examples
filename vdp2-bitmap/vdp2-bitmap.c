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

static void _vblank_in_handler(void);

struct vdp2_scrn_rotation_table {
        /* Screen start coordinates */
        uint32_t xst;
        uint32_t yst;
        uint32_t zst;

        /* Screen vertical coordinate increments (per each line) */
        uint32_t delta_xst;
        uint32_t delta_yst;

        /* Screen horizontal coordinate increments (per each dot) */
        uint32_t delta_x;
        uint32_t delta_y;

        /* Rotation matrix
         *
         * A B C
         * D E F
         * G H I
         */
        union {
                uint32_t raw[2][3];     /* XXX: Needs to be tested */

                struct {
                        uint32_t a;
                        uint32_t b;
                        uint32_t c;
                        uint32_t d;
                        uint32_t e;
                        uint32_t f;
                } param __packed;

                uint32_t params[6];     /* Parameters A, B, C, D, E, F */
        } matrix;

        /* View point coordinates */
        uint16_t px;
        uint16_t py;
        uint16_t pz;

        unsigned int :16;

        /* Center coordinates */
        uint16_t cx;
        uint16_t cy;
        uint16_t cz;

        unsigned int :16;

        /* Amount of horizontal shifting */
        uint32_t mx;
        uint32_t my;

        /* Scaling coefficients */
        uint32_t kx;            /* Expansion/reduction coefficient X */
        uint32_t ky;            /* Expansion/reduction coefficient Y */

        /* Coefficient table address */
        uint32_t kast;          /* Coefficient table start address */
        uint32_t delta_kast;    /* Addr. increment coeff. table (per line) */
        uint32_t delta_kax;     /* Addr. increment coeff. table (per dot) */
} __packed;

int
main(void)
{
        vdp2_init();

        scu_ic_mask_chg(IC_MASK_ALL, IC_MASK_VBLANK_IN);
        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_IN, _vblank_in_handler);
        scu_ic_mask_chg(~(IC_MASK_VBLANK_IN), IC_MASK_NONE);

        struct scrn_bitmap_format format;
        memset(&format, 0x00, sizeof(format));

        format.sbf_scroll_screen = SCRN_RBG0;
        format.sbf_cc_count = SCRN_CCC_RGB_32768;
        format.sbf_bitmap_size.width = 512;
        format.sbf_bitmap_size.height = 256;
        format.sbf_color_palette = 0x00000000;
        format.sbf_bitmap_pattern = VRAM_ADDR_4MBIT(0, 0x00000);
        format.sbf_rp_mode = 0;
        format.sbf_sf_type = SCRN_SF_TYPE_NONE;
        format.sbf_sf_code = SCRN_SF_CODE_A;
        format.sbf_sf_mode = 0;

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(SCRN_RBG0, 7);
        vdp2_scrn_display_set(SCRN_RBG0, /* no_trans = */ false);

        struct vram_ctl *vram_ctl;
        vram_ctl = vdp2_vram_control_get();

        vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[3].t0 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vdp2_vram_control_set(vram_ctl);

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

        tga_image_decode(&tga, (void *)VRAM_ADDR_4MBIT(0, 0x00000));

        color_rgb555_t bs_color;
        bs_color = COLOR_RGB555(0x05, 0x05, 0x07);

        struct vdp2_scrn_rotation_table *rot_tbl __unused;
        rot_tbl = (struct vdp2_scrn_rotation_table *)VRAM_ADDR_4MBIT(2, 0x00000);

        memset(rot_tbl, 0x00, sizeof(*rot_tbl));

        rot_tbl->xst = 0;
        rot_tbl->yst = 0;
        rot_tbl->zst = 0;

        rot_tbl->delta_xst = 0x00000000;
        rot_tbl->delta_yst = 0x00010000;

        rot_tbl->delta_x = 0x00010000;
        rot_tbl->delta_y = 0x00000000;

        rot_tbl->matrix.param.a = 0x00010000;
        rot_tbl->matrix.param.b = 0x00000000;
        rot_tbl->matrix.param.c = 0x00000000;
        rot_tbl->matrix.param.d = 0x00000000;
        rot_tbl->matrix.param.e = 0x00010000;
        rot_tbl->matrix.param.f = 0x00000000;

        rot_tbl->px = 0;
        rot_tbl->py = 0;
        rot_tbl->pz = 0;

        rot_tbl->cx = 0;
        rot_tbl->cy = 0;
        rot_tbl->cz = 0;

        rot_tbl->mx = 0;
        rot_tbl->my = 0;

        rot_tbl->kx = 0x00010000;
        rot_tbl->ky = 0x00010000;

        rot_tbl->kast = 0;
        rot_tbl->delta_kast = 0;
        rot_tbl->delta_kax = 0;

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE), bs_color);

        vdp2_sprite_priority_set(0, 0);

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A, TVMD_VERT_240);
        vdp2_tvmd_display_set();

        while (true) {
                vdp2_tvmd_vblank_out_wait();
                vdp2_tvmd_vblank_in_wait();

                /* Scale */
                rot_tbl->matrix.raw[0][0] = 0x00019980; /* int(1.6*1024.0)<<6 */
                rot_tbl->matrix.raw[1][1] = 0x00011100; /* int(1.0666666666666667*1024.0)<<6 */

                /* Translate */
                rot_tbl->mx += 0x00010000;
                rot_tbl->my += 0x00010000;

                vdp2_commit();
        }

        return 0;
}

static void
_vblank_in_handler(void)
{
        dma_queue_flush(DMA_QUEUE_TAG_VBLANK_IN);

        /* Remove this hack. We have to wait until SCU DMA is done
         * copying the buffered VDP2 registers to memory. */
        scu_dma_level0_wait();

        MEMORY_WRITE(16, VDP2(RAMCTL), 0x030F);
        MEMORY_WRITE(16, VDP2(RPTAU), 0x0002);
        MEMORY_WRITE(16, VDP2(RPTAL), ((uint32_t)VRAM_ADDR_4MBIT(2, 0x00000)) & 0xFFFE);
}
