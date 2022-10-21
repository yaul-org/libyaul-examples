/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <tga.h>

#define RBG0_BPD                VDP2_VRAM_ADDR(0, 0x00000)
#define RBG0_RP_TABLE     VDP2_VRAM_ADDR(2, 0x00000)

#define BACK_SCREEN             VDP2_VRAM_ADDR(3, 0x1FFFE)

extern uint8_t asset_bitmap_tga[];

static const vdp2_scrn_bitmap_format_t _bitmap_format = {
        .scroll_screen = VDP2_SCRN_RBG0,
        .ccc           = VDP2_SCRN_CCC_RGB_32768,
        .bitmap_size   = VDP2_SCRN_BITMAP_SIZE_512X256,
        .palette_base  = 0x00000000,
        .bitmap_base   = RBG0_BPD
};

static const vdp2_vram_usage_t _vram_usage = {
        .a0 = VDP2_VRAM_USAGE_TYPE_CPD_BPD,
        .a1 = VDP2_VRAM_USAGE_TYPE_CPD_BPD,
        .b0 = VDP2_VRAM_USAGE_TYPE_NONE,
        .b1 = VDP2_VRAM_USAGE_TYPE_NONE
};

static const vdp2_scrn_rotation_params_t _rotation_params = {
        .rp_mode       = VDP2_SCRN_RP_MODE_0,
        .rp_table_base = RBG0_RP_TABLE
};

int
main(void)
{
        tga_t tga;
        int32_t ret __unused;
        ret = tga_read(&tga, asset_bitmap_tga);
        assert(ret == TGA_FILE_OK);

        (void)tga_image_decode(&tga, (void *)RBG0_BPD);

        vdp2_scrn_rp_table_t rp_table = {
                /* Screen start coordinates */
                .xst = 0,
                .yst = 0,
                .zst = 0,

                /* Screen vertical coordinate increments (per each line) */
                .delta_xst = FIX16(0.0f),
                .delta_yst = FIX16(1.0f),

                /* Screen horizontal coordinate increments (per each dot) */
                .delta_x = FIX16(1.0f),
                .delta_y = FIX16(0.0f),

                /* Rotation matrix */
                .matrix = {
                        .param = {
                                .a = FIX16(1.0f),
                                .b = FIX16(0.0f),
                                .c = FIX16(0.0f),
                                .d = FIX16(0.0f),
                                .e = FIX16(1.0f),
                                .f = FIX16(0.0f)
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
                .kx = FIX16(1.0f),
                .ky = FIX16(1.0f),

                /* Coefficient table start address */
                .kast = 0x00000000,
                /* Addr. increment coeff. table (per line) */
                .delta_kast = 0,
                /* Addr. increment coeff. table (per dot) */
                .delta_kax = 0
        };

        while (true) {
                /* Scale */
                rp_table.matrix.raw[0][0] = 0x00019980; /* int(1.6*1024.0)<<6 */
                rp_table.matrix.raw[1][1] = 0x00011100; /* int(1.0666666666666667*1024.0)<<6 */

                /* Translate */
                rp_table.mx += FIX16(1.0f);
                rp_table.my += FIX16(1.0f);

                vdp_dma_enqueue((void *)RBG0_RP_TABLE,
                    &rp_table,
                    sizeof(vdp2_scrn_rp_table_t));

                vdp2_sync();
                vdp2_sync_wait();
        }

        return 0;
}

void
user_init(void)
{
        vdp2_scrn_bitmap_format_set(&_bitmap_format);
        vdp2_scrn_rotation_rp_table_set(&_rotation_params);
        vdp2_vram_usage_set(&_vram_usage);

        vdp2_scrn_priority_set(VDP2_SCRN_RBG0, 7); /*  */
        vdp2_scrn_display_set(VDP2_SCRN_RBG0_DISP);

        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 5, 5, 7));

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
