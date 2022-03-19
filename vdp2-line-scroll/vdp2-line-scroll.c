#include <yaul.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define NBG0_CPD         VDP2_VRAM_ADDR(0, 0x00000)
#define NBG0_PND         VDP2_VRAM_ADDR(0, 0x02000)
#define NBG0_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define NBG0_LINE_SCROLL VDP2_VRAM_ADDR(0, 0x03000)

#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x1FFFE)

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

extern uint8_t asset_vf_cpd[];
extern uint8_t asset_vf_cpd_end[];
extern uint8_t asset_vf_pnd[];
extern uint8_t asset_vf_pnd_end[];
extern uint8_t asset_vf_pal[];
extern uint8_t asset_vf_pal_end[];

static vdp2_scrn_ls_h_t _line_scroll_table[SCREEN_HEIGHT];

void
main(void)
{
        vdp2_scrn_ls_format_t ls_format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .table         = NBG0_LINE_SCROLL,
                .interval      = 0,
                .enable        = VDP2_SCRN_LS_HORZ
        };

        fix16_t speed;
        speed = FIX16(0.0f);

        const fix16_t amplitude = FIX16(120.0f);

        while (true) {
                for (uint32_t i = 0; i < SCREEN_HEIGHT; i++) {
                        const fix16_t value = fix16_mul(fix16_int32_from(i), FIX16(M_PI / (float)SCREEN_HEIGHT)) + speed;

                        _line_scroll_table[i].horz = fix16_mul(amplitude, fix16_sin(value));
                }

                speed += FIX16(M_PI / (float)SCREEN_HEIGHT);

                if (speed >= FIX16(2 * M_PI)) {
                        speed = FIX16(0.0f);
                }

                vdp2_scrn_ls_set(&ls_format);

                vdp_dma_enqueue((void *)NBG0_LINE_SCROLL,
                    _line_scroll_table,
                    sizeof(vdp2_scrn_ls_h_t) * SCREEN_HEIGHT);

                vdp2_sync();
                vdp2_sync_wait();
        }
}

static void
_vblank_out_handler(void *work __unused)
{
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

        vdp2_scrn_cell_format_set(&format);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_NBG0_TPDISP);

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        const vdp2_vram_cycp_bank_t vram_cycp_bank_a0 = {
                .t0 = VDP2_VRAM_CYCP_PNDR_NBG0,
                .t1 = VDP2_VRAM_CYCP_CPU_RW,
                .t2 = VDP2_VRAM_CYCP_CPU_RW,
                .t3 = VDP2_VRAM_CYCP_CPU_RW,
                .t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .t6 = VDP2_VRAM_CYCP_CPU_RW,
                .t7 = VDP2_VRAM_CYCP_CPU_RW
        };

        vdp2_vram_cycp_bank_set(0, &vram_cycp_bank_a0);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_color_set(BACK_SCREEN, COLOR_RGB1555(1, 5, 5, 7));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        scu_dma_transfer(0, (void *)NBG0_CPD, asset_vf_cpd, asset_vf_cpd_end - asset_vf_cpd);
        scu_dma_transfer(0, (void *)NBG0_PND, asset_vf_pnd, asset_vf_pnd_end - asset_vf_pnd);
        scu_dma_transfer(0, (void *)NBG0_PAL, asset_vf_pal, asset_vf_pal_end - asset_vf_pal);
        scu_dma_transfer_wait(0);

        vdp2_tvmd_display_set();
}
