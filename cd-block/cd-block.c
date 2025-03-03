/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdlib.h>

#include <scroll_menu.h>

#define MENU_ENTRY_COUNT (16)

static void _frt_ovi_handler(void);
static void _vblank_out_handler(void *work);

static void _menu_input(scroll_menu_t *menu);
static void _menu_update(scroll_menu_t *menu);
static void _menu_action(void *work, menu_entry_t *menu_entry);

static menu_entry_t _menu_entries[MENU_ENTRY_COUNT];

static smpc_peripheral_digital_t _digital;

static cdfs_filelist_t _filelist;

static uint16_t _frt_overflow_count = 0;

int
main(void)
{
        /* Load the maximum number. We have to free the allocated filelist
         * entries, but since we never exit, we don't have to */
        cdfs_filelist_entry_t * const filelist_entries =
            cdfs_entries_alloc(-1);
        assert(filelist_entries != NULL);

        cdfs_config_default_set();
        cdfs_filelist_init(&_filelist, filelist_entries, -1);
        cdfs_filelist_root_read(&_filelist);

        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        scroll_menu_t menu;

        scroll_menu_init(&menu);
        scroll_menu_input_set(&menu, _menu_input);
        scroll_menu_update_set(&menu, _menu_update);
        scroll_menu_entries_set(&menu, _menu_entries, MENU_ENTRY_COUNT);

        menu.view_height = MENU_ENTRY_COUNT;
        menu.top_index = 0;
        menu.bottom_index = _filelist.entries_count - 1;

        menu.flags = SCROLL_MENU_ENABLED | SCROLL_MENU_INPUT_ENABLED;

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                dbgio_printf("[H[2J");

                scroll_menu_update(&menu);

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

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        cpu_frt_init(CPU_FRT_CLOCK_DIV_128);

        cd_block_init();

        smpc_peripheral_init();

        vdp2_tvmd_display_set();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_frt_ovi_handler(void)
{
        _frt_overflow_count++;
}

static void
_menu_input(scroll_menu_t *menu)
{
        if ((_digital.held.button.down) != 0) {
                scroll_menu_cursor_down(menu);
        } else if ((_digital.held.button.up) != 0) {
                scroll_menu_cursor_up(menu);
        } else if ((_digital.held.button.a) != 0) {
                scroll_menu_action_call(menu);
        }
}

static void
_menu_update(scroll_menu_t *menu)
{
        for (uint8_t i = 0; i < menu->view_height; i++) {
                menu_entry_t * const menu_entry = &_menu_entries[i];

                const uint32_t y = scroll_menu_local_cursor(menu) + i;

                char * const name = _filelist.entries[y].name;

                if ((y >= _filelist.entries_count) || (name == NULL) || (*name == '\0')) {
                        *menu_entry->label = '\0';
                        menu_entry->action_fn = NULL;

                        continue;
                }

                strncpy(menu_entry->label, name, sizeof(menu_entry->label));
                menu_entry->action_fn = _menu_action;
        }
}

static void
_menu_action(void *state_ptr, menu_entry_t *menu_entry __unused)
{
        scroll_menu_t *menu;
        menu = state_ptr;

        uint32_t i = scroll_menu_cursor(menu);

        cdfs_filelist_entry_t *file_entry;
        file_entry = &_filelist.entries[i];

        dbgio_printf("\n\nLoading %s, FAD: %li, %i sectors...\n",
            file_entry->name,
            file_entry->starting_fad,
            file_entry->sector_count);

        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();

        cpu_frt_ovi_set(_frt_ovi_handler);

        cpu_frt_count_set(0);

        /* Reset overflow counter after setting the FRT count to zero in case
         * there's an FRT overflow interrupt */
        _frt_overflow_count = 0;

        int ret __unused;
        ret = cd_block_sectors_read(file_entry->starting_fad, (void *)LWRAM(0), file_entry->size);
        assert(ret == 0);

        uint32_t ticks_count;
        ticks_count = (65536 * _frt_overflow_count) + cpu_frt_count_get();

        /* Use Q28.4 to calculate time in milliseconds */
        uint32_t time;
        time = ((ticks_count << 8) / ((1000 * CPU_FRT_NTSC_320_128_COUNT_1MS) << 4)) >> 4;

        dbgio_printf("\n\nLoaded! Took %lu ticks (~%lus).\n\nCheck LWRAM.\n\nWaiting 5 seconds\n", ticks_count, time);
        dbgio_flush();

        vdp2_sync();
        vdp2_sync_wait();

        vdp2_tvmd_vblank_in_next_wait(5);
}
