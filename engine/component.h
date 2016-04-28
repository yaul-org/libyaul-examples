/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_COMPONENT_H
#define ENGINE_COMPONENT_H

#include <math.h>
#include <stdbool.h>
#include <inttypes.h>

struct object;

#define COMPONENT_DECLARATIONS                                                 \
    bool active;                                                               \
    const struct object *object;                                               \
    /* Events */                                                               \
    void (*on_init)(void);                                                     \
    void (*on_update)(void);                                                   \
    void (*on_draw)(void);                                                     \
    void (*on_destroy)(void);

#define COMPONENT_CALL_EVENT(x, name, args...) do {                            \
        if (((struct component *)(x))->CC_CONCAT(on_, name) != NULL) {         \
            if (((struct component *)(x))->active) {                           \
                    ((struct component *)(x))->CC_CONCAT(on_, name)(##args);   \
            }                                                                  \
        }                                                                      \
} while (false)

#define COMPONENT(x, member)                                                   \
        ((x)->CC_CONCAT(, member))

#define COMPONENT_COMPONENT(x, component)                                      \
        ((x)->CC_CONCAT(, component))

#define COMPONENT_PUBLIC_DATA(x, member)                                       \
        ((x)->data.CC_CONCAT(m_, member))

#define COMPONENT_CALL_PUBLIC_MEMBER(x, name, args...)                         \
        ((x))->functions.CC_CONCAT(m_, name)((struct component *)(x), ##args)

struct component {
        COMPONENT_DECLARATIONS
} __may_alias;

#endif /* !ENGINE_COMPONENT_H */
