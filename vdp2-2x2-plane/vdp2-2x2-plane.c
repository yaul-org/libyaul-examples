/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdbool.h>

extern uint8_t root_romdisk[];

static void _hardware_init(void);

int
main(void)
{
        _hardware_init();

        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        struct scrn_cell_format format;

        uint16_t *color_palette;
        color_palette = (uint16_t *)CRAM_MODE_1_OFFSET(0, 0, 0);

        uint16_t *planes[4];
        planes[0] = (uint16_t *)VRAM_ADDR_4MBIT(1, 0x00000);
        planes[1] = (uint16_t *)VRAM_ADDR_4MBIT(1, 0x08000);
        planes[2] = (uint16_t *)VRAM_ADDR_4MBIT(1, 0x10000);
        planes[3] = (uint16_t *)VRAM_ADDR_4MBIT(1, 0x18000);
        uint16_t *cpd;
        cpd = (uint16_t *)VRAM_ADDR_4MBIT(0, 0x00000);

        format.scf_scroll_screen = SCRN_NBG1;
        format.scf_cc_count = SCRN_CCC_PALETTE_256;
        format.scf_character_size = 1 * 1;
        format.scf_pnd_size = 1; /* 1-word */
        format.scf_auxiliary_mode = 0;
        format.scf_plane_size = 2 * 2;
        format.scf_cp_table = (uint32_t)cpd;
        format.scf_color_palette = (uint32_t)color_palette;
        format.scf_map.plane_a = (uint32_t)planes[0];
        format.scf_map.plane_b = (uint32_t)planes[1];
        format.scf_map.plane_c = (uint32_t)planes[2];
        format.scf_map.plane_d = (uint32_t)planes[3];

        struct vram_cycp vram_cycp;

        vram_cycp.pt[0].t0 = VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp.pt[0].t1 = VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp.pt[0].t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[1].t0 = VRAM_CYCP_PNDR_NBG1;
        vram_cycp.pt[1].t1 = VRAM_CYCP_PNDR_NBG1;
        vram_cycp.pt[1].t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[2].t0 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t1 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[3].t0 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t1 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t7 = VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_set(&vram_cycp);

        uint16_t page_size;
        page_size = SCRN_CALCULATE_PAGE_SIZE(&format);

        struct dma_level_cfg dma_level_cfg;
        struct dma_reg_buffer reg_buffer;

        dma_level_cfg.dlc_mode = DMA_MODE_DIRECT;
        dma_level_cfg.dlc_stride = DMA_STRIDE_2_BYTES;
        dma_level_cfg.dlc_update = DMA_UPDATE_NONE;

        void *fh[3];
        void *p;
        uint32_t len;
        int8_t ret;

        {
                fh[0] = romdisk_open(romdisk, "/PAGE.PAL");
                assert(fh[0] != NULL);
                p = romdisk_direct(fh[0]);
                len = romdisk_total(fh[0]);

                dma_level_cfg.dlc_xfer.direct.len = len;
                dma_level_cfg.dlc_xfer.direct.dst = (uint32_t)color_palette;
                dma_level_cfg.dlc_xfer.direct.src = (uint32_t)p;
                scu_dma_config_buffer(&reg_buffer, &dma_level_cfg);

                ret = dma_queue_enqueue(&reg_buffer, DMA_QUEUE_TAG_VBLANK_IN,
                    NULL, NULL);
                assert(ret == 0);
        }

        {
                fh[1] = romdisk_open(romdisk, "/PAGE.CPD");
                assert(fh[1] != NULL);
                p = romdisk_direct(fh[1]);
                len = romdisk_total(fh[1]);

                dma_level_cfg.dlc_xfer.direct.len = len;
                dma_level_cfg.dlc_xfer.direct.dst = (uint32_t)cpd;
                dma_level_cfg.dlc_xfer.direct.src = (uint32_t)p;
                scu_dma_config_buffer(&reg_buffer, &dma_level_cfg);

                ret = dma_queue_enqueue(&reg_buffer, DMA_QUEUE_TAG_VBLANK_IN,
                    NULL, NULL);
                assert(ret == 0);
        }

        {
                fh[2] = romdisk_open(romdisk, "/PAGE.MAP");
                assert(fh[2] != NULL);
                p = romdisk_direct(fh[2]);
                len = romdisk_total(fh[2]);

                /* Traverse through 4 planes, each of 2x2 pages */
                uint32_t j;
                for (j = 0; j < 4; j++) {
                        uint32_t page;
                        page = (uint32_t)planes[j];

                        uint32_t i;
                        for (i = 0; i < (2 * 2); i++) {
                                struct dma_xfer *xfer;
                                xfer = &dma_level_cfg.dlc_xfer.direct;

                                xfer->len = len;
                                xfer->dst = (uint32_t)page;
                                xfer->src = (uint32_t)p;

                                scu_dma_config_buffer(&reg_buffer, &dma_level_cfg);

                                ret = dma_queue_enqueue(&reg_buffer, DMA_QUEUE_TAG_VBLANK_IN, NULL, NULL);
                                assert(ret == 0);

                                page += page_size;
                        }
                }
        }

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);
        romdisk_close(fh[2]);

        vdp2_scrn_cell_format_set(&format);
        vdp2_scrn_priority_set(SCRN_NBG1, 7);
        vdp2_scrn_scroll_x_set(SCRN_NBG1, F16(0.0f));
        vdp2_scrn_scroll_y_set(SCRN_NBG1, F16(0.0f));

        vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ true);

        vdp_sync(0);

        vdp2_tvmd_display_set();

        while (true) {
                vdp2_scrn_scroll_x_update(SCRN_NBG1, F16(4.0f));
                vdp2_scrn_scroll_y_update(SCRN_NBG1, F16(4.0f));

                vdp_sync(0);
        }
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
