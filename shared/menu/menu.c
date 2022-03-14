/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>
#include <stdlib.h>

#include <yaul.h>

#include "menu.h"

static void _menu_render_update(menu_state_t *);

static inline void __always_inline
_menu_cursor_clamp(menu_state_t *menu_state, int8_t dir)
{
        int8_t cursor = menu_state->_cursor + dir;

        if (cursor < 0) {
                cursor = 0;
        } else if (cursor >= menu_state->_entries_count) {
                cursor = menu_state->_entries_count - 1;
        }

        menu_state->_cursor = cursor;
}

void
menu_init(menu_state_t *menu_state)
{
        menu_state->entries = NULL;
        menu_state->flags = MENU_STATE_NONE;
        menu_state->data = NULL;

        menu_state->_cursor = 0;
        menu_state->_entries_count = 0;
        menu_state->_input_fn = NULL;
}

void
menu_input_set(menu_state_t *menu_state, menu_fn_t input_fn)
{
        menu_state->_input_fn = input_fn;
}

void
menu_entries_set(menu_state_t *menu_state, menu_entry_t *entries)
{
        menu_state->entries = entries;
}

void
menu_update(menu_state_t *menu_state)
{
        if ((menu_state->flags & MENU_STATE_ENABLED) != MENU_STATE_ENABLED) {
                return;
        }

        menu_state->current_entry = &menu_state->entries[menu_state->_cursor];

        _menu_render_update(menu_state);

        if (menu_state->_input_fn != NULL) {
                menu_state->_input_fn(menu_state);
        }
}

menu_cursor_t
menu_cursor(menu_state_t *menu_state)
{
        return menu_state->_cursor;
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

        menu_entry_t *menu_entry = &menu_state->entries[menu_state->_cursor];
        menu_action_t action = menu_entry->action;

        if (action != NULL) {
                action(menu_state, menu_entry);
        }
}

static void
_menu_render_update(menu_state_t *menu_state)
{
        static char cursor_buffer[2] = {
                '',
                '\0'
        };

        const menu_entry_t *entry_ptr = menu_state->entries;

        menu_state->_entries_count = 0;

        for (menu_cursor_t cursor = 0; entry_ptr->text != NULL; cursor++, entry_ptr++) {
                cursor_buffer[0] = (menu_state->_cursor == cursor) ? '' : ' ';

                dbgio_puts(cursor_buffer);

                dbgio_puts(entry_ptr->text);
                dbgio_puts("\n");

                menu_state->_entries_count++;
        }
}
