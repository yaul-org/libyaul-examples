#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define RBG0_CPD         VDP2_VRAM_ADDR(0, 0x00000)
#define RBG0_PND         VDP2_VRAM_ADDR(1, 0x02000)
#define RBG0_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define RBG0_RP_TABLE    VDP2_VRAM_ADDR(1, 0x00000)
#define RBG0_COEFF_TABLE 0x00000000

#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x1FFFE)

extern uint8_t asset_cpd[];
extern uint8_t asset_cpd_end[];
extern uint8_t asset_pnd[];
extern uint8_t asset_pnd_end[];
extern uint8_t asset_pal[];
extern uint8_t asset_pal_end[];

static const vdp2_scrn_rp_table_t _rp_table = {
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
        .kast = RBG0_COEFF_TABLE,
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

        scu_dma_transfer(0, (void *)RBG0_RP_TABLE, (void *)&_rp_table, sizeof(_rp_table));

        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }
}

void
user_init(void)
{
        const vdp2_scrn_cell_format_t format = {
                .scroll_screen = VDP2_SCRN_RBG0_PA,
                .ccc           = VDP2_SCRN_CCC_PALETTE_256,
                .char_size     = VDP2_SCRN_CHAR_SIZE_2X2,
                .pnd_size      = 1,
                .aux_mode      = VDP2_SCRN_AUX_MODE_0,
                .plane_size    = VDP2_SCRN_PLANE_SIZE_1X1,
                .cpd_base      = RBG0_CPD,
                .palette_base  = RBG0_PAL
        };

        const vdp2_scrn_rotation_map_t rotation_map = {
                .single = true,
                .plane  = RBG0_PND
        };

        const vdp2_scrn_rotation_params_t rotation_params = {
                .rp_mode       = VDP2_SCRN_RP_MODE_0,
                .rp_table      = &_rp_table,
                .rp_table_base = RBG0_RP_TABLE
        };

        const vdp2_vram_usage_t vram_usage = {
                .a0 = VDP2_VRAM_USAGE_TYPE_CPD_BPD,
                .a1 = VDP2_VRAM_USAGE_TYPE_PND,
                .b0 = VDP2_VRAM_USAGE_TYPE_NONE,
                .b1 = VDP2_VRAM_USAGE_TYPE_NONE
        };

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 5, 5, 7));

        vdp2_scrn_rotation_cell_format_set(&format, &rotation_map);
        vdp2_scrn_rotation_rp_table_set(&rotation_params);
        vdp2_vram_usage_set(&vram_usage);

        vdp2_scrn_priority_set(VDP2_SCRN_RBG0, 3);
        vdp2_scrn_display_set(VDP2_SCRN_DISP_RBG0);

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
