#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "menu.h"

#define NBG0_CPD         VDP2_VRAM_ADDR(0, 0x00000)
#define NBG0_PND         VDP2_VRAM_ADDR(0, 0x13000)
#define NBG0_PAL         VDP2_CRAM_MODE_1_OFFSET(1, 0, 0)
#define NBG0_LINE_SCROLL VDP2_VRAM_ADDR(0, 0x14000)

#define LNCL_SCREEN      VDP2_VRAM_ADDR(0, 0x15000)
#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x1FFFE)

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

#define INCR_VALUE FIX16(0.1f)

extern uint8_t asset_nbg0_cpd[];
extern uint8_t asset_nbg0_cpd_end[];
extern uint8_t asset_nbg0_pnd[];
extern uint8_t asset_nbg0_pnd_end[];
extern uint8_t asset_nbg0_pal[];
extern uint8_t asset_nbg0_pal_end[];

static void _lncl_init(void);

static void _vblank_out_handler(void *work);

static void _menu_input(menu_t *menu);

static void _menu_zoom_cycle(void *work, menu_entry_t *menu_entry, int32_t direction);
static void _menu_zoom_scale_cycle(void *work, menu_entry_t *menu_entry, int32_t direction);
static void _menu_scroll_x_cycle(void *work, menu_entry_t *menu_entry, int32_t direction);
static void _menu_scroll_y_cycle(void *work, menu_entry_t *menu_entry, int32_t direction);

static void _menu_zoom_update(void *work, menu_entry_t *menu_entry);
static void _menu_zoom_scale_update(void *work, menu_entry_t *menu_entry);
static void _menu_scroll_x_update(void *work, menu_entry_t *menu_entry);
static void _menu_scroll_y_update(void *work, menu_entry_t *menu_entry);

static void _menu_reset(void *work, menu_entry_t *menu_entry);

static smpc_peripheral_digital_t _digital;

static menu_entry_t _menu_entries[] = {
        MENU_CYCLE_ENTRY( "Zoom       %f", _menu_zoom_cycle,       _menu_zoom_update),
        MENU_CYCLE_ENTRY( "Zoom Scale %f", _menu_zoom_scale_cycle, _menu_zoom_scale_update),
        MENU_CYCLE_ENTRY( "Scroll X   %f", _menu_scroll_x_cycle,   _menu_scroll_x_update),
        MENU_CYCLE_ENTRY( "Scroll Y   %f", _menu_scroll_y_cycle,   _menu_scroll_y_update),
        MENU_ACTION_ENTRY(" Reset",            _menu_reset)
};

typedef struct values {
        fix16_vec2_t scroll;
        fix16_vec2_t scroll_amount;
        fix16_t zoom;
        fix16_t zoom_scale;
} values_t;

static const values_t _default_values = {
        .scroll        = FIX16_VEC2_INITIALIZER(0.0f, 0.0f),
        .scroll_amount = FIX16_VEC2_INITIALIZER(0.5f, 0.0f),
        .zoom          = FIX16(0.7f),
        .zoom_scale    = FIX16(0.3f)
};

static values_t _values;

static uint16_t _lncl_buffer[SCREEN_HEIGHT];

void
main(void)
{
        vdp2_scrn_ls_format_t ls_format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .table_base    = NBG0_LINE_SCROLL,
                .interval      = 0,
                .type          = VDP2_SCRN_LS_TYPE_VERT | VDP2_SCRN_LS_TYPE_HORZ | VDP2_SCRN_LS_TYPE_ZOOM_HORZ
        };

        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        /* XXX: Hack: Modify the font palette directly */
        MEMORY_WRITE(16, VDP2_CRAM_ADDR(0x0007), 0xFFFF);

        menu_t state;

        menu_init(&state);
        menu_entries_set(&state, _menu_entries, sizeof(_menu_entries) / sizeof(*_menu_entries));
        menu_input_set(&state, _menu_input);

        state.data = NULL;
        state.flags = MENU_ENABLED | MENU_INPUT_ENABLED;

        _values = _default_values;

        volatile vdp2_scrn_ls_hvz_t *_line_scroll_tbl =
            (volatile vdp2_scrn_ls_hvz_t *)NBG0_LINE_SCROLL;

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                dbgio_printf("[H[2J[2;1H");

                menu_update(&state);

                for (uint32_t i = 0; i < SCREEN_HEIGHT; i++) {
                        const angle_t value = fix16_mul(fix16_int32_from(i), FIX16(1.0f / (2.0f * (float)SCREEN_HEIGHT)));

                        const fix16_t shift = fix16_mul(FIX16(-SCREEN_WIDTH * 0.5f), _values.zoom_scale);
                        const fix16_t sin = fix16_sin(value);

                        _line_scroll_tbl[i].horz = _values.scroll.x + fix16_mul(shift, sin);
                        _line_scroll_tbl[i].vert = _values.scroll.y + fix16_int32_from((SCREEN_HEIGHT - 1) - i);
                        _line_scroll_tbl[i].horz_incr = _values.zoom + fix16_mul(_values.zoom_scale, sin);
                }

                vdp2_scrn_ls_set(&ls_format);
                dbgio_flush();

                vdp2_sync();
                vdp2_sync_wait();

                _values.scroll.x += _values.scroll_amount.x;
                _values.scroll.y += _values.scroll_amount.y;
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

        smpc_peripheral_init();

        vdp2_scrn_cell_format_set(&format, &normal_map);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 6);
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

        _lncl_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 0, 0, 0));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        scu_dma_transfer(0, (void *)NBG0_CPD, asset_nbg0_cpd, asset_nbg0_cpd_end - asset_nbg0_cpd);
        scu_dma_transfer(0, (void *)NBG0_PND, asset_nbg0_pnd, asset_nbg0_pnd_end - asset_nbg0_pnd);
        scu_dma_transfer(0, (void *)NBG0_PAL, asset_nbg0_pal, asset_nbg0_pal_end - asset_nbg0_pal);
        scu_dma_transfer_wait(0);

        vdp2_tvmd_display_set();
}

static void
_lncl_init(void)
{
        volatile rgb1555_t *cram_gradient =
            (volatile rgb1555_t *)VDP2_CRAM_MODE_1_OFFSET(3, 0, 0);

        for (uint32_t i = 0; i < 32; i++) {
                cram_gradient[i] = RGB1555(1, i, i, i);
        }

        for (uint32_t i = 0; i < SCREEN_HEIGHT; i++) {
                const angle_t value = fix16_mul(fix16_int32_from(i), FIX16(1.0f / (2.0f * (float)SCREEN_HEIGHT)));
                const uint16_t cram_base = (uintptr_t)cram_gradient >> 1;
                const uint16_t cram_offset = (31 - fix16_int32_to(fix16_mul(FIX16(31.0f), fix16_sin(value))));

                _lncl_buffer[i] = cram_base + cram_offset;
        }

        vdp2_scrn_lncl_buffer_set(LNCL_SCREEN, _lncl_buffer, SCREEN_HEIGHT);
        vdp2_scrn_lncl_set(VDP2_SCRN_NBG0);
        vdp2_scrn_lncl_sync();

        vdp2_ioregs_t * const vdp2_regs = vdp2_regs_get();

        vdp2_regs->ccctl  = (1 << 0) | (1 << 5);
        vdp2_regs->ccrna  = 16;
        vdp2_regs->ccrlb  = 8;
        vdp2_regs->lnclen = 1 << 0;
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_menu_reset(void *work __unused, menu_entry_t *menu_entry __unused)
{
        _values = _default_values;
}

static void
_menu_zoom_update(void *work __unused, menu_entry_t *menu_entry)
{
        (void)sprintf(menu_entry->label, menu_entry->label_format, _values.zoom);
}

static void
_menu_zoom_cycle(void *work __unused, menu_entry_t *menu_entry __unused, int32_t direction)
{
        _values.zoom += fix16_int16_mul(INCR_VALUE, direction);
}

static void
_menu_zoom_scale_update(void *work __unused, menu_entry_t *menu_entry)
{
        (void)sprintf(menu_entry->label, menu_entry->label_format, _values.zoom_scale);
}

static void
_menu_zoom_scale_cycle(void *work __unused, menu_entry_t *menu_entry __unused, int32_t direction)
{
        _values.zoom_scale += fix16_int16_mul(INCR_VALUE, direction);
}

static void
_menu_scroll_x_update(void *work __unused, menu_entry_t *menu_entry)
{
        (void)sprintf(menu_entry->label, menu_entry->label_format, _values.scroll_amount.x);
}

static void
_menu_scroll_x_cycle(void *work __unused, menu_entry_t *menu_entry __unused, int32_t direction)
{
        _values.scroll_amount.x += fix16_int16_mul(INCR_VALUE, direction);
}

static void
_menu_scroll_y_update(void *work __unused, menu_entry_t *menu_entry)
{
        (void)sprintf(menu_entry->label, menu_entry->label_format, _values.scroll_amount.y);
}

static void
_menu_scroll_y_cycle(void *work __unused, menu_entry_t *menu_entry __unused, int32_t direction)
{
        _values.scroll_amount.y += fix16_int16_mul(INCR_VALUE, direction);
}

static void
_menu_input(menu_t *menu)
{
        if (_digital.held.button.down) {
                menu_cursor_down_move(menu);
        } else if (_digital.held.button.up) {
                menu_cursor_up_move(menu);
        } else if (_digital.held.button.left) {
                menu_cycle_call(menu, -1);
        } else if (_digital.held.button.right) {
                menu_cycle_call(menu,  1);
        } else if (_digital.held.button.a || _digital.held.button.c) {
                menu_action_call(menu);
        }
}
