#ifndef _SHARED_MENU_MENU_H_
#define _SHARED_MENU_MENU_H_

#include <stdint.h>
#include <stddef.h>

#define MENU_ENTRY(_text, _action)                                              \
{                                                                               \
        .text = _text,                                                          \
        .action = _action                                                       \
}

#define MENU_END MENU_ENTRY(NULL, NULL)

typedef struct menu_state menu_state_t;

typedef struct menu_entry menu_entry_t;

typedef void (*menu_action_t)(menu_state_t *, menu_entry_t *);

typedef void (*menu_fn_t)(menu_state_t *);

struct menu_entry {
        const char *text;
        menu_action_t action;
};

typedef enum {
        MENU_STATE_ENABLED       = 1 << 0,
        MENU_STATE_INPUT_ENABLED = 1 << 1,
} menu_state_flags_t;

struct menu_state {
        menu_entry_t *entries;
        menu_fn_t input_fn;  
        int8_t cursor;
        menu_state_flags_t flags;
        void *data;

        /* Private */
        int8_t _entries_count;
};

void menu_cursor_up(menu_state_t *);
void menu_cursor_down(menu_state_t *);
void menu_action_call(menu_state_t *);

void menu_update(menu_state_t *);

#endif /* _SHARED_MENU_MENU_H_ */
