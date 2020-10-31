/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdlib.h>

#include "scroll_menu.h"

#define MENU_ENTRY_COUNT 16

static void _vblank_out_handler(void *);

static void _hardware_init(void);

static void _menu_input(scroll_menu_state_t *);
static void _menu_update(scroll_menu_state_t *);

static smpc_peripheral_digital_t _digital;

static iso9660_filelist_t _filelist;
static iso9660_filelist_entry_t _filelist_entries[ISO9660_FILELIST_ENTRIES_COUNT];

static menu_entry_t _menu_entries[MENU_ENTRY_COUNT + 1];

int
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        _filelist.entries = _filelist_entries;
        _filelist.entries_count = 0;
        _filelist.entries_pooled_count = 0;

        iso9660_filelist_read(&_filelist, -1);

        scroll_menu_state_t menu_state;

        scroll_menu_init(&menu_state);
        scroll_menu_input_set(&menu_state, _menu_input);
        scroll_menu_update_set(&menu_state, _menu_update);
        scroll_menu_entries_set(&menu_state, _menu_entries);

        menu_state.view_height = MENU_ENTRY_COUNT - 1;
        menu_state.top_index = 0;
        menu_state.bottom_index = _filelist.entries_count;

        menu_state.flags = SCROLL_MENU_STATE_ENABLED | SCROLL_MENU_STATE_INPUT_ENABLED;

        _menu_entries[MENU_ENTRY_COUNT].text = NULL;
        _menu_entries[MENU_ENTRY_COUNT].action = NULL;

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                dbgio_printf("[H[2J");

                scroll_menu_update(&menu_state);

                dbgio_flush();
                vdp_sync();
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();

        int ret;
        ret = cd_block_init(0x0002);
        assert(ret == 0);

        if ((cd_block_cmd_is_auth(NULL)) == 0) {
                ret = cd_block_bypass_copy_protection();
                assert(ret == 0);
        }
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_menu_input(scroll_menu_state_t *menu_state)
{
        if ((_digital.held.button.down) != 0) {
                scroll_menu_cursor_down(menu_state);
        } else if ((_digital.held.button.up) != 0) {
                scroll_menu_cursor_up(menu_state);
        } else if ((_digital.held.button.a) != 0) {
                scroll_menu_action_call(menu_state);
        }
}

static void
_menu_update(scroll_menu_state_t *menu_state)
{
        for (uint32_t i = 0; i <= (MENU_ENTRY_COUNT - 1); i++) {
                menu_entry_t *menu_entry;
                menu_entry = &_menu_entries[i];

                uint32_t y;
                y = scroll_menu_cursor(menu_state) + i;

                menu_entry->text = _filelist.entries[y].name;
                menu_entry->action = NULL;
        }
}
