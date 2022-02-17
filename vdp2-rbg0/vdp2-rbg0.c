#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define RBG0_CPD                VDP2_VRAM_ADDR(0, 0x10000)
#define RBG0_PAL                VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define RBG0_PND                VDP2_VRAM_ADDR(1, 0x00000)
#define RBG0_ROTATION_TABLE     VDP2_VRAM_ADDR(2, 0x00000)

#define BACK_SCREEN             VDP2_VRAM_ADDR(3, 0x1FFFE)

extern uint8_t asset_cpd[];
extern uint8_t asset_cpd_end[];
extern uint8_t asset_pnd[];
extern uint8_t asset_pnd_end[];
extern uint8_t asset_pal[];
extern uint8_t asset_pal_end[];

static const vdp2_scrn_rotation_table_t _rotation_table = {
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
        .kast = 0,
        /* Addr. increment coeff. table (per line) */
        .delta_kast = 0,
        /* Addr. increment coeff. table (per dot) */
        .delta_kax = 0
};

void
main(void)
{
        scu_dma_transfer(0, (void *)RBG0_CPD, asset_cpd, asset_cpd_end - asset_cpd);
        scu_dma_transfer(0, (void *)RBG0_PND, asset_pnd, asset_pnd_end - asset_pnd);
        scu_dma_transfer(0, (void *)RBG0_PAL, asset_pal, asset_pal_end - asset_pal);

        scu_dma_transfer(0, (void *)RBG0_ROTATION_TABLE, (void *)&_rotation_table, sizeof(_rotation_table));

        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }
}

void
user_init(void)
{
        const vdp2_scrn_cell_format_t format = {
                .scroll_screen  = VDP2_SCRN_RBG0,
                .cc_count       = VDP2_SCRN_CCC_PALETTE_256,
                .character_size = 2 * 2,
                .pnd_size       = 1, /* 1-word */
                .auxiliary_mode = 0,
                .plane_size     = 2 * 2,
                .cp_table       = RBG0_CPD,
                .color_palette  = RBG0_PAL,
                .map_bases      = {
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
                .rotation_table = RBG0_ROTATION_TABLE,
                .usage_banks    = {
                        .a0 = VDP2_VRAM_USAGE_TYPE_CPD,
                        .a1 = VDP2_VRAM_USAGE_TYPE_PND,
                        .b0 = VDP2_VRAM_USAGE_TYPE_NONE,
                        .b1 = VDP2_VRAM_USAGE_TYPE_NONE
                }
        };

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(BACK_SCREEN, COLOR_RGB1555(1, 5, 5, 7));

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
