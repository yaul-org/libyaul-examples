/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdlib.h>

#include "menu.h"

static void _vblank_out_handler(void *);

static void _menu_input(menu_t *menu);

static void _input_1(void *work, menu_entry_t *menu_entry);
static void _input_2(void *work, menu_entry_t *menu_entry);

static smpc_peripheral_digital_t _digital;

static menu_entry_t _menu_entries[] = {
        MENU_ACTION_ENTRY("Item 1", _input_1),
        MENU_ACTION_ENTRY("Item 2", _input_2),
        MENU_ACTION_ENTRY("Item 3", NULL),
        MENU_ACTION_ENTRY("Item 4", NULL),
        MENU_ACTION_ENTRY("Item 5", NULL)
};

int
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        menu_t state;

        menu_init(&state);
        menu_entries_set(&state, _menu_entries, sizeof(_menu_entries) / sizeof(*_menu_entries));
        menu_input_set(&state, _menu_input);

        state.data = NULL;

        state.flags = MENU_ENABLED | MENU_INPUT_ENABLED;

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                dbgio_printf("[H[2J");

                menu_update(&state);

                dbgio_flush();
                vdp2_sync();
                vdp2_sync_wait();
        }
}

void
user_init(void)
{
        smpc_peripheral_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp2_tvmd_display_set();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_menu_input(menu_t *menu)
{
        if ((_digital.held.button.down) != 0) {
                menu_cursor_down_move(menu);
        } else if ((_digital.held.button.up) != 0) {
                menu_cursor_up_move(menu);
        } else if ((_digital.held.button.a) != 0) {
                menu_action_call(menu);
        }
}

static void
_input_1(void *state __unused, menu_entry_t *menu_entry __unused)
{
        dbgio_printf("Input 1 pressed\n");
}

static void
_input_2(void *state __unused, menu_entry_t *menu_entry __unused)
{
        dbgio_printf("Input 2 pressed\n");
}
