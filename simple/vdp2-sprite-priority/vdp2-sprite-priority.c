/*
 * Copyright (c) 2012-2018 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <sys/queue.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NBG1_CPD                VRAM_ADDR_4MBIT(0, 0x00000)
#define NBG1_PAL                CRAM_MODE_1_OFFSET(1, 0, 0)
#define NBG1_MAP_PLANE_A        VRAM_ADDR_4MBIT(0, 0x14000)
#define NBG1_MAP_PLANE_B        VRAM_ADDR_4MBIT(0, 0x14000)
#define NBG1_MAP_PLANE_C        VRAM_ADDR_4MBIT(0, 0x14000)
#define NBG1_MAP_PLANE_D        VRAM_ADDR_4MBIT(0, 0x14000)

extern uint8_t root_romdisk[];

static void _hardware_init(void);

static void _dma_upload(void *, void *, size_t);

void
main(void)
{
        _hardware_init();

        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        void *fh[3];
        void *p;
        size_t len;

        fh[0] = romdisk_open(romdisk, "CHR.DAT");
        assert(fh[0] != NULL);
        p = romdisk_direct(fh[0]);
        len = romdisk_total(fh[0]);
        _dma_upload((void *)VRAM_ADDR_4MBIT(0, 0x00000), p, len);

        fh[1] = romdisk_open(romdisk, "PND.DAT");
        assert(fh[1] != NULL);
        p = romdisk_direct(fh[1]);
        len = romdisk_total(fh[1]);
        _dma_upload((void *)VRAM_ADDR_4MBIT(0, 0x14000), p, len);

        fh[2] = romdisk_open(romdisk, "PAL.DAT");
        assert(fh[2] != NULL);
        p = romdisk_direct(fh[2]);
        len = romdisk_total(fh[2]);
        _dma_upload((void *)CRAM_ADDR(0x0000), p, len);

        vdp2_tvmd_display_set();

        vdp2_sync_commit();
        vdp_sync(0);

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);
        romdisk_close(fh[2]);

        while (true) {
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_clear();

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(2, 0x01FFFE),
            COLOR_RGB555(0, 0, 7));

        const struct scrn_cell_format format = {
                .scf_scroll_screen = SCRN_NBG1,
                .scf_cc_count = SCRN_CCC_PALETTE_256,
                .scf_character_size = 1 * 1,
                .scf_pnd_size = 1,
                .scf_auxiliary_mode = 1,
                .scf_sf_type = SCRN_SF_TYPE_NONE,
                .scf_sf_code = SCRN_SF_CODE_A,
                .scf_plane_size = 1 * 1,
                .scf_cp_table = NBG1_CPD,
                .scf_color_palette = NBG1_PAL,
                .scf_map = {
                        .planes = {
                                NBG1_MAP_PLANE_A,
                                NBG1_MAP_PLANE_B,
                                NBG1_MAP_PLANE_C,
                                NBG1_MAP_PLANE_D
                        }
                }
        };

        vdp2_scrn_cell_format_set(&format);
        vdp2_scrn_priority_set(SCRN_NBG1, 6);
        vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ false);

        struct vram_cycp_bank vram_cycp_bank;

        vram_cycp_bank.t0 = VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank.t1 = VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank.t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t4 = VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank.t5 = VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank.t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t7 = VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_bank_set(0, &vram_cycp_bank);
        vdp2_vram_cycp_bank_clear(1);
        vdp2_vram_cycp_bank_clear(2);
        vdp2_vram_cycp_bank_clear(3);

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);

        vdp2_sync_commit();
        vdp_sync(0);
}

static void
_dma_upload(void *dst, void *src, size_t len)
{
        struct dma_level_cfg dma_level_cfg;
        struct dma_reg_buffer reg_buffer;

        dma_level_cfg.dlc_mode = DMA_MODE_DIRECT;
        dma_level_cfg.dlc_stride = DMA_STRIDE_2_BYTES;
        dma_level_cfg.dlc_update = DMA_UPDATE_NONE;
        dma_level_cfg.dlc_xfer.direct.len = len;
        dma_level_cfg.dlc_xfer.direct.dst = (uint32_t)dst;
        dma_level_cfg.dlc_xfer.direct.src = CPU_CACHE_THROUGH | (uint32_t)src;

        scu_dma_config_buffer(&reg_buffer, &dma_level_cfg);

        int8_t ret;
        ret = dma_queue_enqueue(&reg_buffer, DMA_QUEUE_TAG_VBLANK_IN, NULL, NULL);
        assert(ret == 0);
}
