/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdlib.h>

#include <yaul.h>

#include "scroll_menu.h"

static void _scroll(scroll_menu_state_t *, int8_t);

static void _menu_input(menu_state_t *);

void
scroll_menu_init(scroll_menu_state_t *scroll_menu_state)
{
        scroll_menu_state->view_height = 0;
        scroll_menu_state->top_index = 0;
        scroll_menu_state->bottom_index = 0;

        scroll_menu_state->entries = NULL;
        scroll_menu_state->flags = SCROLL_MENU_STATE_NONE;
        scroll_menu_state->data = NULL;

        scroll_menu_state->_cursor = 0;
        scroll_menu_state->_y = 0;
        scroll_menu_state->_gp = 0;

        menu_state_t *menu_state;
        menu_state = &scroll_menu_state->_menu_state;

        menu_init(menu_state);
        menu_input_set(menu_state, _menu_input);

        menu_state->data = scroll_menu_state;
}

void
scroll_menu_input_set(scroll_menu_state_t *menu_state, scroll_menu_fn_t input_fn)
{
        menu_state->_input_fn = input_fn;

        assert(menu_state->_input_fn != NULL);
}

void
scroll_menu_update_set(scroll_menu_state_t *menu_state, scroll_menu_fn_t update_fn)
{
        menu_state->_update_fn = update_fn;
}

void
scroll_menu_entries_set(scroll_menu_state_t *scroll_menu_state, menu_entry_t *entries)
{
        scroll_menu_state->entries = entries;

        menu_state_t *menu_state;
        menu_state = &scroll_menu_state->_menu_state;

        menu_entries_set(menu_state, entries);
}

menu_cursor_t
scroll_menu_local_cursor(scroll_menu_state_t *menu_state)
{
        return menu_state->_y;
}

menu_cursor_t
scroll_menu_cursor(scroll_menu_state_t *menu_state)
{
        return menu_state->_cursor;
}

void
scroll_menu_cursor_up(scroll_menu_state_t *menu_state)
{
        menu_cursor_up(&menu_state->_menu_state);

        _scroll(menu_state, -1);
}

void
scroll_menu_cursor_down(scroll_menu_state_t *menu_state)
{
        menu_cursor_down(&menu_state->_menu_state);

        _scroll(menu_state, 1);
}

void
scroll_menu_action_call(scroll_menu_state_t *menu_state)
{
        if ((menu_state->flags & MENU_STATE_ENABLED) != MENU_STATE_ENABLED) {
                return;
        }

        menu_cursor_t cursor;
        cursor = menu_state->_gp;

        menu_entry_t *menu_entry = &menu_state->entries[cursor];
        menu_action_t action = menu_entry->action;

        if (action != NULL) {
                action(menu_state, menu_entry);
        }
}

void
scroll_menu_update(scroll_menu_state_t *scroll_menu_state)
{
        if (scroll_menu_state->_update_fn != NULL) {
                scroll_menu_state->_update_fn(scroll_menu_state);
        }

        menu_state_t *menu_state;
        menu_state = &scroll_menu_state->_menu_state;

        /* Update flags */
        menu_state->flags &= ~MENU_STATE_MASK;
        menu_state->flags |= scroll_menu_state->flags & MENU_STATE_MASK;

        menu_update(menu_state);
}

static void
_scroll(scroll_menu_state_t *menu_state, int8_t dir)
{
        menu_state->_cursor += dir;

        if (menu_state->_cursor < 0) {
                menu_state->_cursor = 0;
        } else if (menu_state->_cursor > menu_state->bottom_index) {
                menu_state->_cursor = menu_state->bottom_index;
        }

        menu_state->_gp += dir;

        if (menu_state->_gp < 0) {
                menu_state->_gp = 0;

                menu_state->_y += dir;
        } else if (menu_state->_gp > menu_state->view_height) {
                menu_state->_gp = menu_state->view_height;

                menu_state->_y += dir;
        }

        if (menu_state->_y < menu_state->top_index) {
                menu_state->_y = menu_state->top_index;
        } else if ((menu_state->_y + menu_state->view_height) >= menu_state->bottom_index) {
                menu_state->_y = menu_state->bottom_index - menu_state->view_height - 1;
        }
}

static void
_menu_input(menu_state_t *menu_state)
{
        scroll_menu_state_t *scroll_menu_state;
        scroll_menu_state = menu_state->data;

        if (scroll_menu_state->_input_fn != NULL) {
                scroll_menu_state->_input_fn(scroll_menu_state);
        }
}
