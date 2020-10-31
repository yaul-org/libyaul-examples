#ifndef _SHARED_MENU_MENU_H_
#define _SHARED_MENU_MENU_H_

#include <stdint.h>
#include <stddef.h>

#define MENU_ENTRY(_text, _action)                                             \
{                                                                              \
        .text = _text,                                                         \
        .action = _action                                                      \
}

#define MENU_END MENU_ENTRY(NULL, NULL)

typedef uint8_t menu_cursor_t;

typedef struct menu_state menu_state_t;

typedef struct menu_entry menu_entry_t;

typedef void (*menu_action_t)(menu_state_t *, menu_entry_t *);

typedef void (*menu_fn_t)(menu_state_t *);

struct menu_entry {
        const char *text;
        menu_action_t action;
};

#define MENU_STATE_MASK (0x0003)

typedef enum {
        MENU_STATE_NONE          = 0,
        MENU_STATE_ENABLED       = 1 << 0,
        MENU_STATE_INPUT_ENABLED = 1 << 1,
} menu_state_flags_t;

struct menu_state {
        menu_entry_t *entries;
        menu_state_flags_t flags;
        void *data;

        /* Private */
        int8_t _cursor;
        int8_t _entries_count;
        menu_fn_t _input_fn;  
};

void menu_init(menu_state_t *);
void menu_input_set(menu_state_t *, menu_fn_t);
void menu_entries_set(menu_state_t *, menu_entry_t *);
void menu_update(menu_state_t *);

menu_cursor_t menu_cursor(menu_state_t *);
void menu_cursor_up(menu_state_t *);
void menu_cursor_down(menu_state_t *);
void menu_action_call(menu_state_t *);

#endif /* _SHARED_MENU_MENU_H_ */
