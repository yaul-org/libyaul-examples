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

static void _hardware_init(void);

int
main(void)
{
        _hardware_init();

        void *romdisk;

        romdisk_init();

        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        vdp2_scrn_bitmap_format_t format;
        memset(&format, 0x00, sizeof(format));

        format.scroll_screen = VDP2_SCRN_NBG0;
        format.cc_count = VDP2_SCRN_CCC_PALETTE_256;
        format.bitmap_size.width = 1024;
        format.bitmap_size.height = 512;
        format.color_palette = VDP2_CRAM_ADDR(0x300);
        format.bitmap_pattern = VDP2_VRAM_ADDR(0, 0x00000);
        format.sf_type = VDP2_SCRN_SF_TYPE_NONE;
        format.sf_code = VDP2_SCRN_SF_CODE_A;
        format.sf_mode = 0;

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_reduction_set(VDP2_SCRN_NBG0, VDP2_SCRN_REDUCTION_HALF);
        vdp2_scrn_reduction_x_set(VDP2_SCRN_NBG0, Q0_3_8(2.0f));
        vdp2_scrn_reduction_y_set(VDP2_SCRN_NBG0, Q0_3_8(1.0f / (1.0f - (240.0f / 512.0f))));
        vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* no_trans = */ false);

        vdp2_vram_cycp_t vram_cycp;

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

        vram_cycp.pt[2].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[3].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_set(&vram_cycp);

        /* Set for CRAM mode 1: RGB 555 2,048 colors */
        vdp2_cram_mode_set(1);

        /* Set up and enqueue two DMA transfers tagged as VBLANK-IN */

        scu_dma_level_cfg_t scu_dma_level_cfg;
        scu_dma_reg_buffer_t reg_buffer;

        scu_dma_level_cfg.mode = SCU_DMA_MODE_DIRECT;
        scu_dma_level_cfg.stride = SCU_DMA_STRIDE_2_BYTES;
        scu_dma_level_cfg.update = SCU_DMA_UPDATE_NONE;

        void *fh[2];
        void *p;
        int8_t ret;

        fh[0] = romdisk_open(romdisk, "/BITMAP.PAL");
        assert(fh[0] != NULL);

        p = romdisk_direct(fh[0]);

        scu_dma_level_cfg.xfer.direct.len = romdisk_total(fh[0]);
        scu_dma_level_cfg.xfer.direct.dst = VDP2_CRAM_ADDR(0x0000);
        scu_dma_level_cfg.xfer.direct.src = (uint32_t)p;

        scu_dma_config_buffer(&reg_buffer, &scu_dma_level_cfg);
        ret = dma_queue_enqueue(&reg_buffer, DMA_QUEUE_TAG_VBLANK_IN, NULL, NULL);
        assert(ret == 0);

        fh[1] = romdisk_open(romdisk, "/BITMAP.CPD");
        assert(fh[1] != NULL);

        p = romdisk_direct(fh[1]);

        scu_dma_level_cfg.xfer.direct.len = romdisk_total(fh[1]);
        scu_dma_level_cfg.xfer.direct.dst = VDP2_VRAM_ADDR(0, 0x00000);
        scu_dma_level_cfg.xfer.direct.src = (uint32_t)p;

        scu_dma_config_buffer(&reg_buffer, &scu_dma_level_cfg);
        ret = dma_queue_enqueue(&reg_buffer, DMA_QUEUE_TAG_VBLANK_IN, NULL, NULL);
        assert(ret == 0);

        vdp_sync();

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);

        while (true) {
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}
