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

#include "component.h"

#include "camera.h"
#include "collider.h"
#include "transform.h"
#include "rigid_body.h"

#define OBJECT_COMPONENT_LIST_MAX       8

#define OBJECT_ID_RESERVED_0            0x8000
#define OBJECT_ID_RESERVED_1            0x8001
#define OBJECT_ID_RESERVED_2            0x8002
#define OBJECT_ID_RESERVED_3            0x8003
#define OBJECT_ID_RESERVED_4            0x8004
#define OBJECT_ID_RESERVED_5            0x8005
#define OBJECT_ID_RESERVED_6            0x8006
#define OBJECT_ID_RESERVED_7            0x8007
#define OBJECT_ID_RESERVED_8            0x8008
#define OBJECT_ID_RESERVED_9            0x8009
#define OBJECT_ID_RESERVED_10           0x800A
#define OBJECT_ID_RESERVED_11           0x800B
#define OBJECT_ID_RESERVED_12           0x800C
#define OBJECT_ID_RESERVED_13           0x800D
#define OBJECT_ID_RESERVED_14           0x800E
#define OBJECT_ID_RESERVED_15           0x800F

#define OBJECT_ID_RESERVED_BEGIN        0x8000
#define OBJECT_ID_RESERVED_END          0xFFFF

#define OBJECT_DECLARATIONS                                                    \
bool active;                                                                   \
    int32_t id;                                                                \
    /* Enable rendering or not */                                              \
    bool visible;                                                              \
    /* Vertex list is clockwise:                                               \
     * 1--2                                                                    \
     * |  |                                                                    \
     * 0--3                                                                    \
     * \_ Origin                                                               \
     * Subsequent 4 vertices is another polygon. */                            \
    fix16_vector3_t *vertex_list;                                              \
    uint32_t vertex_count;                                                     \
    /* Color list is one color per 4 vertices, dependent on                    \
     * vertex_count */                                                         \
    color_rgb555_t *color_list;                                                \
    /* Builtin Components */                                                   \
    struct transform transform;                                                \
    struct camera *camera;                                                     \
    struct collider *colliders;                                                \
    struct rigid_body *rigid_body;                                             \
    struct component *component_list[OBJECT_COMPONENT_LIST_MAX];               \
    uint32_t component_count;                                                  \
    /* Events */                                                               \
    void (*on_init)(struct object *);                                          \
    void (*on_update)(struct object *);                                        \
    void (*on_draw)(struct object *);                                          \
    void (*on_destroy)(struct object *);                                       \
    void (*on_collision)(struct object *, const struct object *,               \
        const struct collider_info *);                                         \
    void (*on_trigger)(struct object *, const struct object *);                \
                                                                               \
    /* If it's been initialized or not (call to "init" event) */               \
    bool initialized;                                                          \
    /* Context used by objects system */                                       \
    const void *context;

#define OBJECT_EVENT(x, name, args...) do {                                    \
        if (((struct object *)(x))->CC_CONCAT(on_, name) != NULL) {            \
                if (OBJECT(((struct object *)(x)), active)) {                  \
                        /* We must have the object initialized before          \
                         * calling any other event */                          \
                        assert(OBJECT(((struct object *)(x)), initialized));   \
                        ((struct object *)(x))->CC_CONCAT(on_, name)(          \
                                (struct object *)(x), ##args);                 \
                }                                                              \
        }                                                                      \
} while (false)

#define OBJECT_INIT(x, args...) do {                                           \
        assert(((struct object *)(x))->on_init != NULL);                       \
        ((struct object *)(x))->on_init((struct object *)(x), ##args);         \
        /* Mark object Initialized */                                          \
        OBJECT(((struct object *)(x)), initialized) = true;                    \
} while (false)

#define OBJECT_UPDATE(x)                OBJECT_EVENT(x, update)
#define OBJECT_DRAW(x)                  OBJECT_EVENT(x, draw)
#define OBJECT_DESTROY(x)               OBJECT_EVENT(x, destroy)
#define OBJECT_COLLISION(x, args...)    OBJECT_EVENT(x, collision, ##args)
#define OBJECT_TRIGGER(x, args...)      OBJECT_EVENT(x, trigger, ##args)

#define OBJECT(x, member)                                                      \
        ((x)->CC_CONCAT(, member))

#define OBJECT_COMPONENT(x, component)                                         \
        ((x)->CC_CONCAT(, component))

#define OBJECT_DATA(x, member)                                                 \
        ((x)->data.CC_CONCAT(m_, member))

#define OBJECT_P_DATA(x, member)                                               \
        ((x)->private_data.CC_CONCAT(m_, member))

#define OBJECT_CALL_FUNCTION(x, name, args...)                                 \
        ((x))->functions.CC_CONCAT(m_, name)((struct object *)(x), ##args)

#define THIS(type, member)                                                     \
        (((struct type *)this)->CC_CONCAT(, member))

#define THIS_COMPONENT(type, component)                                        \
        (((struct type *)this)->CC_CONCAT(, component))

#define THIS_DATA(type, member)                                                \
        (((struct type *)this)->data.CC_CONCAT(m_, member))

#define THIS_P_DATA(type, member)                                              \
        (((struct type *)this)->private_data.CC_CONCAT(m_, member))

#define THIS_CALL_FUNCTION(type, name, args...)                                \
        ((struct type *)this)->functions.CC_CONCAT(m_, name)(this, ##args)

#define THIS_CALL_P_FUNCTION(type, name, args...)                              \
        ((struct type *)this)->private_functions.CC_CONCAT(m_, name)(this,     \
            ##args)

struct object {
        OBJECT_DECLARATIONS
} __may_alias;

extern void object_component_init(const struct object *);
extern void object_component_update(const struct object *);
extern void object_component_draw(const struct object *);

#endif /* !ENGINE_OBJECT_H */
