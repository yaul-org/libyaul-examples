#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define RBG0_CPD                VDP2_VRAM_ADDR(0, 0x10000)
#define RBG0_PAL                VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define RBG0_PND                VDP2_VRAM_ADDR(1, 0x00000)
#define RBG0_ROTATION_TABLE     VDP2_VRAM_ADDR(2, 0x00000)

#define BACK_SCREEN             VDP2_VRAM_ADDR(3, 0x1FFFE)

extern uint8_t root_romdisk[];

static const vdp2_scrn_rotation_table_t _rot_tbl __used = {
        .xst = 0,
        .yst = 0,
        .zst = 0,

        .delta_xst = 0x00000000,
        .delta_yst = 0x00010000,

        .delta_x = 0x00010000,
        .delta_y = 0x00000000,

        .matrix = {
                .param = {
                        .a = 0x00010000,
                        .b = 0x00000000,
                        .c = 0x00000000,
                        .d = 0x00000000,
                        .e = 0x00010000,
                        .f = 0x00000000
                }
        },

        .px = 0,
        .py = 0,
        .pz = 0,

        .cx = 0,
        .cy = 0,
        .cz = 0,

        .mx = 0,
        .my = 0,

        .kx = 0x00010000,
        .ky = 0x00010000,

        .kast = 0,
        .delta_kast = 0,
        .delta_kax = 0,
};

static scu_dma_xfer_t _xfer_table[4] __aligned(4 * 16);

void
main(void)
{
        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        void *fh[3];

        scu_dma_level_cfg_t scu_dma_level_cfg = {
                .xfer.indirect = &_xfer_table[0],
                .mode = SCU_DMA_MODE_INDIRECT,
                .stride = SCU_DMA_STRIDE_2_BYTES,
                .update = SCU_DMA_UPDATE_NONE
        };

        scu_dma_handle_t handle;

        scu_dma_config_buffer(&handle, &scu_dma_level_cfg);

        fh[0] = romdisk_open(romdisk, "/CPD.BIN");
        assert(fh[0] != NULL);
        _xfer_table[0].len = romdisk_total(fh[0]);
        _xfer_table[0].dst = RBG0_CPD;
        _xfer_table[0].src = (uint32_t)romdisk_direct(fh[0]);

        fh[1] = romdisk_open(romdisk, "/PAL.BIN");
        assert(fh[1] != NULL);
        _xfer_table[1].len = romdisk_total(fh[1]);
        _xfer_table[1].dst = RBG0_PAL;
        _xfer_table[1].src = (uint32_t)romdisk_direct(fh[1]);

        fh[2] = romdisk_open(romdisk, "/PND.BIN");
        assert(fh[2] != NULL);
        _xfer_table[2].len = romdisk_total(fh[2]);
        _xfer_table[2].dst = RBG0_PND;
        _xfer_table[2].src = (uint32_t)romdisk_direct(fh[2]);

        _xfer_table[3].len = sizeof(_rot_tbl);
        _xfer_table[3].dst = RBG0_ROTATION_TABLE;
        _xfer_table[3].src = SCU_DMA_INDIRECT_TABLE_END | (uint32_t)&_rot_tbl;

        int8_t ret __unused;
        ret = dma_queue_enqueue(&handle, DMA_QUEUE_TAG_VBLANK_IN,
            NULL, NULL);
        assert(ret == 0);

        vdp2_sync();
        vdp2_sync_wait();

        romdisk_close(fh[2]);
        romdisk_close(fh[1]);
        romdisk_close(fh[0]);

        while (true) {
        }
}

void
user_init(void)
{
        const vdp2_scrn_cell_format_t format = {
                .scroll_screen = VDP2_SCRN_RBG0,
                .cc_count = VDP2_SCRN_CCC_PALETTE_256,
                .character_size = 2 * 2,
                .pnd_size = 1, /* 1-word */
                .auxiliary_mode = 0,
                .plane_size = 2 * 2,
                .cp_table = RBG0_CPD,
                .color_palette = RBG0_PAL,
                .map_bases = {
                        .planes = {
                                RBG0_PND,
                                RBG0_PND,
                                RBG0_PND,
                                RBG0_PND,

                                RBG0_PND,
                                RBG0_PND,
                                RBG0_PND,
                                RBG0_PND,

                                RBG0_PND,
                                RBG0_PND,
                                RBG0_PND,
                                RBG0_PND,

                                RBG0_PND,
                                RBG0_PND,
                                RBG0_PND,
                                RBG0_PND
                        }
                },
                .rotation_table = VDP2_VRAM_ADDR(2, 0x00000),
                .usage_banks = {
                        .a0 = VDP2_VRAM_USAGE_TYPE_CPD,
                        .a1 = VDP2_VRAM_USAGE_TYPE_PND,
                        .b0 = VDP2_VRAM_USAGE_TYPE_NONE,
                        .b1 = VDP2_VRAM_USAGE_TYPE_NONE
                }
        };

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(BACK_SCREEN, COLOR_RGB1555(1, 5, 5, 7));

        vdp2_tvmd_display_clear();

        vdp2_scrn_cell_format_set(&format);

        vdp2_scrn_priority_set(VDP2_SCRN_RBG0, 3);
        vdp2_scrn_display_set(VDP2_SCRN_RBG0, /* transparent = */ false);

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_vram_cycp_clear();

        vdp2_tvmd_display_set();
}
