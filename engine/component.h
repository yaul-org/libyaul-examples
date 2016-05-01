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
    void (*on_init)(struct component *);                                       \
    void (*on_update)(struct component *);                                     \
    void (*on_draw)(struct component *);                                       \
    void (*on_destroy)(struct component *);                                    \
                                                                               \
    /* If it's been initialized or not (call to "init" event) */               \
    bool initialized;

#define COMPONENT_EVENT(x, name, args...) do {                                 \
        if (((struct component *)(x))->CC_CONCAT(on_, name) != NULL) {         \
                if (COMPONENT(((struct component *)(x)), active)) {            \
                        /* We must have the component initialized before       \
                         * calling any other event */                          \
                        assert(COMPONENT(((struct component *)(x)), initialized)); \
                        ((struct component *)(x))->CC_CONCAT(on_, name)(       \
                                (struct component *)(x), ##args);              \
                }                                                              \
        }                                                                      \
} while (false)

#define COMPONENT_INIT(x, args...) do {                                        \
        assert(((struct component *)(x))->on_init != NULL);                    \
        ((struct component *)(x))->on_init((struct component *)(x), ##args);   \
        /* Mark component Initialized */                                       \
        COMPONENT(((struct component *)(x)), initialized) = true;              \
} while (false)

#define COMPONENT_UPDATE(x)     COMPONENT_EVENT(x, update)
#define COMPONENT_DRAW(x)       COMPONENT_EVENT(x, draw)
#define COMPONENT_DESTROY(x)    COMPONENT_EVENT(x, destroy)

#define COMPONENT(x, member)                                                   \
        ((x)->CC_CONCAT(, member))

#define COMPONENT_COMPONENT(x, component)                                      \
        ((x)->CC_CONCAT(, component))

#define COMPONENT_PUBLIC_DATA(x, member)                                       \
        ((x)->data.CC_CONCAT(m_, member))

#define COMPONENT_CALL_PUBLIC_MEMBER(x, name, args...)                         \
        ((x))->functions.CC_CONCAT(m_, name)((struct component *)(x), ##args)

#define COMPONENT_THIS(type, member)                                           \
        (((struct type *)this)->CC_CONCAT(, member))

#define COMPONENT_THIS_PUBLIC_DATA(type, member)                               \
        (((struct type *)this)->data.CC_CONCAT(m_, member))

#define COMPONENT_THIS_PRIVATE_DATA(type, member)                              \
        (((struct type *)this)->private_data.CC_CONCAT(m_, member))

struct component {
        COMPONENT_DECLARATIONS
} __may_alias;

#endif /* !ENGINE_COMPONENT_H */
