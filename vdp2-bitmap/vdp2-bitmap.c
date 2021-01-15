/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <tga.h>

#define RBG0_BPD                VDP2_VRAM_ADDR(0, 0x00000)
#define RBG0_ROTATION_TABLE     VDP2_VRAM_ADDR(2, 0x00000)

#define BACK_SCREEN             VDP2_VRAM_ADDR(3, 0x1FFFE)

extern uint8_t root_romdisk[];

static const vdp2_scrn_bitmap_format_t format = {
        .scroll_screen       = VDP2_SCRN_RBG0,
        .cc_count            = VDP2_SCRN_CCC_RGB_32768,
        .bitmap_size         = { 512, 256 },
        .color_palette       = 0x00000000,
        .bitmap_pattern      = RBG0_BPD,
        .rp_mode             = 0,
        .sf_type             = VDP2_SCRN_SF_TYPE_NONE,
        .sf_code             = VDP2_SCRN_SF_CODE_A,
        .sf_mode             = 0,
        .rotation_table_base = RBG0_ROTATION_TABLE,
        .usage_banks         = {
                .a0 = VDP2_VRAM_USAGE_TYPE_BPD,
                .a1 = VDP2_VRAM_USAGE_TYPE_BPD,
                .b0 = VDP2_VRAM_USAGE_TYPE_NONE,
                .b1 = VDP2_VRAM_USAGE_TYPE_NONE
        }
};

int
main(void)
{
        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        void *fh;
        fh = romdisk_open(romdisk, "/BITMAP.TGA");
        assert(fh != NULL);

        tga_t tga;
        int32_t ret __unused;
        ret = tga_read(&tga, romdisk_direct(fh));
        assert(ret == TGA_FILE_OK);

        (void)tga_image_decode(&tga, (void *)RBG0_BPD);

        vdp2_scrn_rotation_table_t rot_tbl = {
                /* Screen start coordinates */
                .xst = 0,
                .yst = 0,
                .zst = 0,

                /* Screen vertical coordinate increments (per each line) */
                .delta_xst = 0x00000000,
                .delta_yst = 0x00010000,

                /* Screen horizontal coordinate increments (per each dot) */
                .delta_x = 0x00010000,
                .delta_y = 0x00000000,

                /* Rotation matrix */
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

                /* View point coordinates */
                .px = 0,
                .py = 0,
                .pz = 0,

                /* Center coordinates */
                .cx = 0,
                .cy = 0,
                .cz = 0,

                /* Amount of horizontal shifting */
                .mx = 0,
                .my = 0,

                /* Scaling coefficients */
                .kx = 0x00010000,
                .ky = 0x00010000,

                /* Coefficient table start address */
                .kast = 0,
                /* Addr. increment coeff. table (per line) */
                .delta_kast = 0,
                /* Addr. increment coeff. table (per dot) */
                .delta_kax = 0
        };

        scu_dma_level_cfg_t scu_dma_level_cfg;
        scu_dma_handle_t handle;

        scu_dma_level_cfg.xfer.direct.len = sizeof(vdp2_scrn_rotation_table_t);
        scu_dma_level_cfg.xfer.direct.dst = RBG0_ROTATION_TABLE;
        scu_dma_level_cfg.xfer.direct.src = (uint32_t)&rot_tbl;
        scu_dma_level_cfg.mode = SCU_DMA_MODE_DIRECT;
        scu_dma_level_cfg.stride = SCU_DMA_STRIDE_2_BYTES;
        scu_dma_level_cfg.update = SCU_DMA_UPDATE_NONE;

        scu_dma_config_buffer(&handle, &scu_dma_level_cfg);

        while (true) {
                /* Scale */
                rot_tbl.matrix.raw[0][0] = 0x00019980; /* int(1.6*1024.0)<<6 */
                rot_tbl.matrix.raw[1][1] = 0x00011100; /* int(1.0666666666666667*1024.0)<<6 */

                /* Translate */
                rot_tbl.mx += 0x00010000;
                rot_tbl.my += 0x00010000;

                int8_t ret __unused;
                ret = dma_queue_enqueue(&handle, DMA_QUEUE_TAG_VBLANK_IN,
                    NULL, NULL);
                assert(ret == 0);

                vdp_sync();
        }

        return 0;
}

void
user_init(void)
{
        /* Force clear the TV display so we can draw */
        vdp2_tvmd_display_clear();
        vdp_sync();

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_RBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_RBG0, /* no_trans = */ false);

        vdp2_vram_cycp_clear();

        vdp2_scrn_back_screen_color_set(BACK_SCREEN, COLOR_RGB1555(1, 5, 5, 7));

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);
        vdp2_tvmd_display_set();
}
