#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NBG0_CPD                VDP2_VRAM_ADDR(0, 0x00000)
#define NBG0_PND                VDP2_VRAM_ADDR(2, 0x00000)
#define NBG0_PAL                VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define NBG0_LINE_SCROLL        VDP2_VRAM_ADDR(1, 0x00000)

#define BACK_SCREEN             VDP2_VRAM_ADDR(3, 0x1FFFE)

extern uint8_t root_romdisk[];

static void _hardware_init(void);

void
main(void)
{
        static struct scu_dma_xfer _xfer_table[4] __aligned(4 * 16);

        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);

        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        void *fh[3];

        const struct scu_dma_level_cfg scu_dma_level_cfg = {
                .xfer.indirect = &_xfer_table[0],
                .mode = SCU_DMA_MODE_INDIRECT,
                .stride = SCU_DMA_STRIDE_2_BYTES,
                .update = SCU_DMA_UPDATE_NONE
        };

        struct scu_dma_reg_buffer reg_buffer;
        scu_dma_config_buffer(&reg_buffer, &scu_dma_level_cfg);

        fh[0] = romdisk_open(romdisk, "/VF.CPD");
        assert(fh[0] != NULL);
        _xfer_table[0].len = romdisk_total(fh[0]);
        _xfer_table[0].dst = NBG0_CPD;
        _xfer_table[0].src = (uint32_t)romdisk_direct(fh[0]);

        fh[1] = romdisk_open(romdisk, "/VF.PND");
        assert(fh[1] != NULL);
        _xfer_table[1].len = romdisk_total(fh[1]);
        _xfer_table[1].dst = NBG0_PND;
        _xfer_table[1].src = (uint32_t)romdisk_direct(fh[1]);

        fh[2] = romdisk_open(romdisk, "/VF.PAL");
        assert(fh[2] != NULL);
        _xfer_table[2].len = romdisk_total(fh[2]);
        _xfer_table[2].dst = NBG0_PAL;
        _xfer_table[2].src = (uint32_t)romdisk_direct(fh[2]);

        fh[3] = romdisk_open(romdisk, "/LINE_SCROLL.TBL");
        assert(fh[3] != NULL);
        _xfer_table[3].len = romdisk_total(fh[3]);
        _xfer_table[3].dst = NBG0_LINE_SCROLL;
        _xfer_table[3].src = SCU_DMA_INDIRECT_TBL_END | (uint32_t)romdisk_direct(fh[3]);

        struct vdp2_scrn_ls_format ls_format = {
                .enable = VDP2_SCRN_LS_N0SCX,
                .interlace_mode = 0,
                .line_scroll_table = NBG0_LINE_SCROLL
        };

        vdp2_scrn_ls_set(&ls_format);

        int8_t ret;
        ret = dma_queue_enqueue(&reg_buffer, DMA_QUEUE_TAG_VBLANK_IN,
            NULL, NULL);
        assert(ret == 0);

        vdp_sync();

        romdisk_close(fh[2]);
        romdisk_close(fh[1]);
        romdisk_close(fh[0]);

        uint16_t i;
        i = 0;

        dbgio_buffer("Line scroll\n");

        while (true) {
                ls_format.line_scroll_table = NBG0_LINE_SCROLL + (i << 2);

                vdp2_scrn_ls_set(&ls_format);
                dbgio_flush();
                vdp_sync();

                i = (i + 1) & 0x00FF;
        }
}

static void
_hardware_init(void)
{
        const struct vdp2_scrn_cell_format format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .cc_count = VDP2_SCRN_CCC_PALETTE_256,
                .character_size = 2 * 2,
                .pnd_size = 1, /* 1-word */
                .auxiliary_mode = 1,
                .plane_size = 1 * 1,
                .cp_table = NBG0_CPD,
                .color_palette = NBG0_PAL,
                .map_bases.plane_a = NBG0_PND,
                .map_bases.plane_b = NBG0_PND,
                .map_bases.plane_c = NBG0_PND,
                .map_bases.plane_d = NBG0_PND
        };

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(BACK_SCREEN, COLOR_RGB555(5, 5, 7));

        vdp2_tvmd_display_clear();

        vdp2_scrn_cell_format_set(&format);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 3);
        vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* transparent = */ false);

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        struct vdp2_vram_cycp_bank vram_cycp_bank;

        vram_cycp_bank.t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank.t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank.t2 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t3 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_bank_set(0, &vram_cycp_bank);

        vram_cycp_bank.t0 = VDP2_VRAM_CYCP_PNDR_NBG0;
        vram_cycp_bank.t1 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t2 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t3 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank.t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_bank_set(2, &vram_cycp_bank);

        vdp2_tvmd_display_set();
}
