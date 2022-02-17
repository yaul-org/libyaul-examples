#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NBG0_CPD                VDP2_VRAM_ADDR(0, 0x00000)
#define NBG0_PND                VDP2_VRAM_ADDR(2, 0x00000)
#define NBG0_PAL                VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define NBG0_LINE_SCROLL        VDP2_VRAM_ADDR(1, 0x00000)

#define BACK_SCREEN             VDP2_VRAM_ADDR(3, 0x1FFFE)

extern uint8_t asset_line_scroll_tbl[];
extern uint8_t asset_line_scroll_tbl_end[];
extern uint8_t asset_vf_cpd[];
extern uint8_t asset_vf_cpd_end[];
extern uint8_t asset_vf_pnd[];
extern uint8_t asset_vf_pnd_end[];
extern uint8_t asset_vf_pal[];
extern uint8_t asset_vf_pal_end[];

static vdp2_scrn_ls_format_t _ls_format = {
        .scroll_screen     = VDP2_SCRN_NBG0,
        .line_scroll_table = NBG0_LINE_SCROLL,
        .interval          = 0,
        .enable            = VDP2_SCRN_LS_HORZ
};

void
main(void)
{
        scu_dma_transfer(0, (void *)NBG0_CPD, asset_vf_cpd, asset_vf_cpd_end - asset_vf_cpd);
        scu_dma_transfer(0, (void *)NBG0_PND, asset_vf_pnd, asset_vf_pnd_end - asset_vf_pnd);
        scu_dma_transfer(0, (void *)NBG0_PAL, asset_vf_pal, asset_vf_pal_end - asset_vf_pal);

        scu_dma_transfer(0, (void *)NBG0_LINE_SCROLL, asset_line_scroll_tbl, asset_line_scroll_tbl_end - asset_line_scroll_tbl);

        vdp2_scrn_ls_set(&_ls_format);

        vdp2_sync();
        vdp2_sync_wait();

        uint16_t i;
        i = 0;

        while (true) {
                _ls_format.line_scroll_table = NBG0_LINE_SCROLL + (i << 2);

                vdp2_scrn_ls_set(&_ls_format);
                vdp2_sync();
                vdp2_sync_wait();

                i = (i + 1) & 0x00FF;
        }
}

void
user_init(void)
{
        const vdp2_scrn_cell_format_t format = {
                .scroll_screen     = VDP2_SCRN_NBG0,
                .cc_count          = VDP2_SCRN_CCC_PALETTE_256,
                .character_size    = 2 * 2,
                .pnd_size          = 1, /* 1-word */
                .auxiliary_mode    = 1,
                .plane_size        = 1 * 1,
                .cp_table          = NBG0_CPD,
                .color_palette     = NBG0_PAL,
                .map_bases.plane_a = NBG0_PND,
                .map_bases.plane_b = NBG0_PND,
                .map_bases.plane_c = NBG0_PND,
                .map_bases.plane_d = NBG0_PND
        };

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(BACK_SCREEN, COLOR_RGB1555(1, 5, 5, 7));

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

        const vdp2_vram_cycp_bank_t vram_cycp_bank_a0 = {
                .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t7 = VDP2_VRAM_CYCP_NO_ACCESS
        };

        vdp2_vram_cycp_bank_set(0, &vram_cycp_bank_a0);

        const vdp2_vram_cycp_bank_t vram_cycp_bank_b0 = {
                .t0 = VDP2_VRAM_CYCP_PNDR_NBG0,
                .t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .t7 = VDP2_VRAM_CYCP_NO_ACCESS
        };

        vdp2_vram_cycp_bank_set(2, &vram_cycp_bank_b0);

        vdp2_tvmd_display_set();
}
