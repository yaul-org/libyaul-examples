/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_OBJECT_H
#define ENGINE_OBJECT_H

#include <math.h>
#include <inttypes.h>

#include "transform.h"

#define OBJECT_COMPONENT_LIST_MAX       16

#define OBJECT_ID_RESERVED_BEGIN        0x8000
#define OBJECT_ID_RESERVED_END          0xFFFF

#define OBJECT_DECLARATIONS                                                    \
        bool active;                                                           \
        int32_t id;                                                            \
        struct component *component_list[OBJECT_COMPONENT_LIST_MAX];           \
        uint32_t component_count;                                              \
        void (*on_destroy)(struct component *);                                \
        /* Context used by objects system */                                   \
        const void *context;

#define OBJECT(x, member)                                                      \
        ((x)->CC_CONCAT(, member))

#define OBJECT_COMPONENT(x, index)                                             \
        ((struct object *)x)->component_list[(index)]

struct object {
        OBJECT_DECLARATIONS
} __may_alias;

extern void object_init(struct object *);
extern void object_destroy(struct object *);
extern void object_update(const struct object *);
extern void object_draw(const struct object *);

#endif /* !ENGINE_OBJECT_H */
