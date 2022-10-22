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

extern uint8_t asset_nbg0_cpd[];
extern uint8_t asset_nbg0_cpd_end[];
extern uint8_t asset_nbg0_pnd[];
extern uint8_t asset_nbg0_pnd_end[];
extern uint8_t asset_nbg0_pal[];
extern uint8_t asset_nbg0_pal_end[];

static vdp2_scrn_ls_h_t _line_scroll_table[SCREEN_HEIGHT];

void
main(void)
{
        vdp2_scrn_ls_format_t ls_format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .table_base    = NBG0_LINE_SCROLL,
                .interval      = 0,
                .type          = VDP2_SCRN_LS_TYPE_HORZ
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

void
user_init(void)
{
        const vdp2_scrn_cell_format_t format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .ccc           = VDP2_SCRN_CCC_PALETTE_256,
                .char_size     = VDP2_SCRN_CHAR_SIZE_2X2,
                .pnd_size      = 1,
                .aux_mode      = VDP2_SCRN_AUX_MODE_1,
                .plane_size    = VDP2_SCRN_PLANE_SIZE_1X1,
                .cpd_base      = NBG0_CPD,
                .palette_base  = NBG0_PAL
        };

        const vdp2_scrn_normal_map_t normal_map = {
                .plane_a = NBG0_PND,
                .plane_b = NBG0_PND,
                .plane_c = NBG0_PND,
                .plane_d = NBG0_PND
        };

        vdp2_scrn_cell_format_set(&format, &normal_map);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_DISPTP_NBG0);

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

        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 5, 5, 7));

        scu_dma_transfer(0, (void *)NBG0_CPD, asset_nbg0_cpd, asset_nbg0_cpd_end - asset_nbg0_cpd);
        scu_dma_transfer(0, (void *)NBG0_PND, asset_nbg0_pnd, asset_nbg0_pnd_end - asset_nbg0_pnd);
        scu_dma_transfer(0, (void *)NBG0_PAL, asset_nbg0_pal, asset_nbg0_pal_end - asset_nbg0_pal);
        scu_dma_transfer_wait(0);

        vdp2_tvmd_display_set();
}
