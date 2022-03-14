#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "menu.h"

#define NBG0_CPD         VDP2_VRAM_ADDR(0, 0x00000)
#define NBG0_PND         VDP2_VRAM_ADDR(0, 0x02000)
#define NBG0_PAL         VDP2_CRAM_MODE_1_OFFSET(1, 0, 0)
#define NBG0_LINE_SCROLL VDP2_VRAM_ADDR(0, 0x03000)

#define LNCL_SCREEN      VDP2_VRAM_ADDR(0, 0x04000)
#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x1FFFE)

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

extern uint8_t asset_vf_cpd[];
extern uint8_t asset_vf_cpd_end[];
extern uint8_t asset_vf_pnd[];
extern uint8_t asset_vf_pnd_end[];
extern uint8_t asset_vf_pal[];
extern uint8_t asset_vf_pal_end[];

static void _lncl_init(void);

static void _vblank_out_handler(void *work);

static void _menu_input(menu_state_t *menu_state);
static void _menu_update(menu_state_t *menu_state);

static smpc_peripheral_digital_t _digital;

static menu_entry_t _menu_entries[] = {
        MENU_ENTRY("Zoom                            ", NULL),
        MENU_ENTRY("Scroll X                        ", NULL),
        MENU_ENTRY("Scroll Y                        ", NULL),
        MENU_END
};

static fix16_vec2_t _scroll = FIX16_VEC2_INITIALIZER(0.0f, 0.0f);
static fix16_vec2_t _scroll_amount = FIX16_VEC2_INITIALIZER(0.5f, 0.0f);
static fix16_t _zoom = FIX16(0.2f);

static uint16_t _lncl_buffer[SCREEN_HEIGHT];

void
main(void)
{
        vdp2_scrn_ls_format_t ls_format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .table         = NBG0_LINE_SCROLL,
                .interval      = 0,
                .enable        = VDP2_SCRN_LS_VERT | VDP2_SCRN_LS_HORZ | VDP2_SCRN_LS_ZOOM_HORZ
        };

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        /* XXX: Hack: Modify the font palette directly */
        MEMORY_WRITE(16, VDP2_CRAM_ADDR(0x0007), 0xFFFF);

        menu_state_t state;

        menu_init(&state);
        menu_entries_set(&state, _menu_entries);
        menu_input_set(&state, _menu_input);

        state.data = NULL;
        state.flags = MENU_STATE_ENABLED | MENU_STATE_INPUT_ENABLED;

        volatile vdp2_scrn_ls_hvz_t *_line_scroll_tbl =
            (volatile vdp2_scrn_ls_hvz_t *)NBG0_LINE_SCROLL;

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                dbgio_printf("[H[2J[2;1H");

                _menu_update(&state);

                for (uint32_t i = 0; i < SCREEN_HEIGHT; i++) {
                        const fix16_t value = fix16_mul(fix16_int32_from(i), FIX16(M_PI / (float)SCREEN_HEIGHT));

                        _line_scroll_tbl[i].horz = _scroll.x + fix16_mul(FIX16(-160), fix16_sin(value));
                        _line_scroll_tbl[i].vert = _scroll.y + fix16_int32_from((SCREEN_HEIGHT - 1) - i);
                        _line_scroll_tbl[i].horz_incr = _zoom + fix16_sin(value);
                }

                vdp2_scrn_ls_set(&ls_format);
                dbgio_flush();

                vdp2_sync();
                vdp2_sync_wait();

                _scroll.x += _scroll_amount.x;
                _scroll.y += _scroll_amount.y;
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

        vdp2_scrn_cell_format_set(&format);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 3);
        vdp2_scrn_display_set(VDP2_SCRN_NBG0_DISP);

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

        _lncl_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_color_set(BACK_SCREEN, COLOR_RGB1555(1, 0, 0, 0));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        scu_dma_transfer(0, (void *)NBG0_CPD, asset_vf_cpd, asset_vf_cpd_end - asset_vf_cpd);
        scu_dma_transfer(0, (void *)NBG0_PND, asset_vf_pnd, asset_vf_pnd_end - asset_vf_pnd);
        scu_dma_transfer(0, (void *)NBG0_PAL, asset_vf_pal, asset_vf_pal_end - asset_vf_pal);
        scu_dma_transfer_wait(0);

        vdp2_tvmd_display_set();
}

static void
_lncl_init(void)
{
        volatile color_rgb1555_t *cram_gradient =
            (volatile color_rgb1555_t *)VDP2_CRAM_MODE_1_OFFSET(3, 0, 0);

        for (uint32_t i = 0; i < 32; i++) {
                cram_gradient[i] = COLOR_RGB1555(1, i, i, i);
        }

        for (uint32_t i = 0; i < SCREEN_HEIGHT; i++) {
                const fix16_t value = fix16_mul(fix16_int32_from(i), FIX16(M_PI / (float)SCREEN_HEIGHT));
                const uint16_t cram_base = (uintptr_t)cram_gradient >> 1;
                const uint16_t cram_offset = (31 - fix16_int32_to(fix16_mul(FIX16(31.0f), fix16_sin(value))));

                _lncl_buffer[i] = cram_base + cram_offset;
        }

        vdp2_scrn_lncl_buffer_set(LNCL_SCREEN, _lncl_buffer, SCREEN_HEIGHT);
        vdp2_scrn_lncl_set(VDP2_SCRN_NBG0);
        vdp2_scrn_lncl_sync();

        vdp2_registers_t * const vdp2_regs = vdp2_regs_get();

        vdp2_regs->ccctl = (1 << 0) | (1 << 5);
        vdp2_regs->ccrna = 16;
        vdp2_regs->ccrlb = 8;
        vdp2_regs->lnclen = 1 << 0;
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_menu_zoom_cycle(menu_entry_t *menu_entry __unused, int32_t direction)
{
        _zoom += fix16_int16_mul(FIX16(0.1f), direction);
}

static void
_menu_scroll_x_cycle(menu_entry_t *menu_entry __unused, int32_t direction)
{
        _scroll_amount.x += fix16_int16_mul(FIX16(0.1f), direction);
}

static void
_menu_scroll_y_cycle(menu_entry_t *menu_entry __unused, int32_t direction)
{
        _scroll_amount.y += fix16_int16_mul(FIX16(0.1f), direction);
}

void
_menu_cycle_input(menu_state_t *menu_state, int32_t direction)
{
        switch (menu_state->_cursor) {
        case 0:
                _menu_zoom_cycle(menu_state->current_entry, direction);
                break;
        case 1:
                _menu_scroll_x_cycle(menu_state->current_entry, direction);
                break;
        case 2:
                _menu_scroll_y_cycle(menu_state->current_entry, direction);
                break;
        }
}

static void
_menu_input(menu_state_t *menu_state)
{
        if ((_digital.held.button.down) != 0) {
                menu_cursor_down(menu_state);
        } else if ((_digital.held.button.up) != 0) {
                menu_cursor_up(menu_state);
        } else if ((_digital.held.button.left) != 0) {
                _menu_cycle_input(menu_state, -1);
        } else if ((_digital.held.button.right) != 0) {
                _menu_cycle_input(menu_state,  1);
        }
}

static void
_menu_update(menu_state_t *menu_state)
{
        sprintf(menu_state->entries[0].text, "Zoom     %f", _zoom);
        sprintf(menu_state->entries[1].text, "Scroll X %f", _scroll_amount.x);
        sprintf(menu_state->entries[2].text, "Scroll Y %f", _scroll_amount.y);

        menu_update(menu_state);
}
