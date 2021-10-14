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

static void _menu_input(menu_state_t *);

static void _input_1(void *, menu_entry_t *);
static void _input_2(void *, menu_entry_t *);

static smpc_peripheral_digital_t _digital;

static menu_entry_t _entries[] = {
        MENU_ENTRY("Item 1", _input_1),
        MENU_ENTRY("Item 2", _input_2),
        MENU_ENTRY("Item 3", NULL),
        MENU_ENTRY("Item 4", NULL),
        MENU_ENTRY("Item 5", NULL),
        MENU_END
};

int
main(void)
{
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        menu_state_t state;

        menu_init(&state);
        menu_entries_set(&state, _entries);
        menu_input_set(&state, _menu_input);

        state.data = NULL;

        state.flags = MENU_STATE_ENABLED | MENU_STATE_INPUT_ENABLED;

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
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_menu_input(menu_state_t *menu_state)
{
        if ((_digital.held.button.down) != 0) {
                menu_cursor_down(menu_state);
        } else if ((_digital.held.button.up) != 0) {
                menu_cursor_up(menu_state);
        } else if ((_digital.held.button.a) != 0) {
                menu_action_call(menu_state);
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
