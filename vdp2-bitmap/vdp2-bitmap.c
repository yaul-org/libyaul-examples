/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <tga.h>

extern uint8_t root_romdisk[];

static void _vblank_in_handler(void);

static void _hardware_init(void);

int
main(void)
{
        _hardware_init();

        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        void *fh;
        fh = romdisk_open(romdisk, "/BITMAP.TGA");
        assert(fh != NULL);

        tga_t tga;
        int32_t ret;
        ret = tga_read(&tga, romdisk_direct(fh));
        assert(ret == TGA_FILE_OK);

        (void)tga_image_decode(&tga, (void *)VRAM_ADDR_4MBIT(0, 0x00000));

        struct scrn_rotation_table rot_tbl = {
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
                .matrix.param.a = 0x00010000,
                .matrix.param.b = 0x00000000,
                .matrix.param.c = 0x00000000,
                .matrix.param.d = 0x00000000,
                .matrix.param.e = 0x00010000,
                .matrix.param.f = 0x00000000,

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

        struct dma_level_cfg dma_level_cfg;
        struct dma_reg_buffer reg_buffer;

        dma_level_cfg.dlc_xfer.direct.len = sizeof(struct scrn_rotation_table);
        dma_level_cfg.dlc_xfer.direct.dst = VRAM_ADDR_4MBIT(2, 0x00000);
        dma_level_cfg.dlc_xfer.direct.src = (uint32_t)&rot_tbl;
        dma_level_cfg.dlc_mode = DMA_MODE_DIRECT;
        dma_level_cfg.dlc_stride = DMA_STRIDE_2_BYTES;
        dma_level_cfg.dlc_update = DMA_UPDATE_NONE;

        scu_dma_config_buffer(&reg_buffer, &dma_level_cfg);

        while (true) {
                /* Scale */
                rot_tbl.matrix.raw[0][0] = 0x00019980; /* int(1.6*1024.0)<<6 */
                rot_tbl.matrix.raw[1][1] = 0x00011100; /* int(1.0666666666666667*1024.0)<<6 */

                /* Translate */
                rot_tbl.mx += 0x00010000;
                rot_tbl.my += 0x00010000;

                int8_t ret;
                ret = dma_queue_enqueue(&reg_buffer, DMA_QUEUE_TAG_VBLANK_IN,
                    NULL, NULL);
                assert(ret == 0);

                vdp_sync(0);
        }

        return 0;
}

static void
_hardware_init(void)
{
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
        format.sbf_rotation_table = VRAM_ADDR_4MBIT(2, 0x00000);
        format.sbf_usage_banks.a0 = VRAM_USAGE_TYPE_BPD;
        format.sbf_usage_banks.a1 = VRAM_USAGE_TYPE_BPD;
        format.sbf_usage_banks.b0 = VRAM_USAGE_TYPE_NONE;
        format.sbf_usage_banks.b1 = VRAM_USAGE_TYPE_NONE;

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(SCRN_RBG0, 7);
        vdp2_scrn_display_set(SCRN_RBG0, /* no_trans = */ false);

        vdp2_vram_cycp_clear();

        vdp_sync_vblank_in_set(_vblank_in_handler);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
                                        COLOR_RGB555(5, 5, 7));

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);
        vdp2_tvmd_display_set();
}

static void
_vblank_in_handler(void)
{
}
