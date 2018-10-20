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

        struct vram_cycp vram_cycp;

        vram_cycp.pt[0].t0 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t1 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t2 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t3 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[1].t0 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t1 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t2 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t3 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[2].t0 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t1 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t2 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t3 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[2].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[3].t0 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t1 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t2 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t3 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[3].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t7 = VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_set(&vram_cycp);

        /* Set up and enqueue two DMA transfers tagged as VBLANK-IN */

        struct dma_level_cfg dma_level_cfg;
        uint8_t reg_buffer[DMA_REG_BUFFER_BYTE_SIZE];

        dma_level_cfg.dlc_mode = DMA_MODE_DIRECT;
        dma_level_cfg.dlc_stride = DMA_STRIDE_2_BYTES;
        dma_level_cfg.dlc_update = DMA_UPDATE_NONE;

        void *fh[2];
        void *p;
        int8_t ret;

        fh[0] = romdisk_open(romdisk, "/BITMAP_PAL.BIN");
        assert(fh[0] != NULL);

        p = romdisk_direct(fh[0]);

        dma_level_cfg.dlc_xfer.direct.len = romdisk_total(fh[0]);
        dma_level_cfg.dlc_xfer.direct.dst = CRAM_ADDR(0x0000);
        dma_level_cfg.dlc_xfer.direct.src = (uint32_t)p;

        scu_dma_config_buffer(&reg_buffer[0], &dma_level_cfg);
        ret = dma_queue_enqueue(&reg_buffer[0], DMA_QUEUE_TAG_VBLANK_IN, NULL, NULL);
        assert(ret == 0);

        fh[1] = romdisk_open(romdisk, "/BITMAP_CPD.BIN");
        assert(fh[1] != NULL);

        p = romdisk_direct(fh[1]);

        dma_level_cfg.dlc_xfer.direct.len = romdisk_total(fh[1]);
        dma_level_cfg.dlc_xfer.direct.dst = VRAM_ADDR_4MBIT(0, 0x00000);
        dma_level_cfg.dlc_xfer.direct.src = (uint32_t)p;

        scu_dma_config_buffer(&reg_buffer[0], &dma_level_cfg);
        ret = dma_queue_enqueue(&reg_buffer[0], DMA_QUEUE_TAG_VBLANK_IN, NULL, NULL);
        assert(ret == 0);

        vdp2_sync_commit();
        vdp_sync(0);

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);

        while (true) {
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}
