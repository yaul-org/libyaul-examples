/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdlib.h>

#include <yaul.h>

#include "menu.h"

static void _menu_render_update(menu_state_t *);

static inline void __always_inline
_menu_cursor_clamp(menu_state_t *menu_state, int8_t dir)
{
        int8_t cursor = menu_state->cursor + dir;

        if (cursor < 0) {
                cursor = 0;
        } else if (cursor >= menu_state->_entries_count) {
                cursor = menu_state->_entries_count - 1;
        }

        menu_state->cursor = cursor;
}

void
menu_update(menu_state_t *menu_state)
{
        if ((menu_state->flags & MENU_STATE_ENABLED) != MENU_STATE_ENABLED) {
                return;
        }

        _menu_render_update(menu_state);

        if (menu_state->input_fn != NULL) {
                menu_state->input_fn(menu_state);                
        }
}

void
menu_cursor_up(menu_state_t *menu_state)
{
        if ((menu_state->flags & MENU_STATE_ENABLED) != MENU_STATE_ENABLED) {
                return;
        }

        _menu_cursor_clamp(menu_state, -1);
}

void
menu_cursor_down(menu_state_t *menu_state)
{
        if ((menu_state->flags & MENU_STATE_ENABLED) != MENU_STATE_ENABLED) {
                return;
        }

        _menu_cursor_clamp(menu_state, 1);
}

void
menu_action_call(menu_state_t *menu_state)
{
        if ((menu_state->flags & MENU_STATE_ENABLED) != MENU_STATE_ENABLED) {
                return;
        }

        menu_entry_t *menu_entry = &menu_state->entries[menu_state->cursor];
        menu_action_t action = menu_entry->action;

        if (action != NULL) {
                action(menu_state, menu_entry);
        }
}

static void
_menu_render_update(menu_state_t *menu_state)
{
        static char cursor_buffer[2];

        const menu_entry_t *entry_ptr = menu_state->entries;

        menu_state->_entries_count = 0;

        for (int8_t i = 0; entry_ptr->text != NULL; i++) {
                const char cursor = (menu_state->cursor == i) ? '' : ' ';

                cursor_buffer[0] = cursor;
                cursor_buffer[1] = '\0';

                dbgio_printf(cursor_buffer);

                dbgio_printf(entry_ptr->text);
                dbgio_printf("\n");

                menu_state->_entries_count++;

                entry_ptr++;
        }
}
