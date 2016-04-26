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

#define COMPONENT_LIST_MAX 8

#define OBJECT_DECLARATIONS                                                    \
    bool active;                                                               \
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
    color_rgb_t *color_list;                                                   \
    /* Builtin Components */                                                   \
    struct transform transform;                                                \
    struct camera *camera;                                                     \
    struct collider *colliders;                                                \
    struct rigid_body *rigid_body;                                             \
    struct component *component_list[COMPONENT_LIST_MAX];                      \
    uint32_t component_count;                                                  \
    /* Events */                                                               \
    void (*on_init)(void);                                                     \
    void (*on_update)(void);                                                   \
    void (*on_draw)(void);                                                     \
    void (*on_destroy)(void);                                                  \
    void (*on_collision)(struct object *, const struct collider_info *);       \
    void (*on_trigger)(struct object *);

#define OBJECT_CALL_EVENT(x, name, args...) do {                               \
        if (((struct object *)(x))->CC_CONCAT(on_, name) != NULL) {            \
            if (((struct object *)(x))->active) {                              \
                    ((struct object *)(x))->CC_CONCAT(on_, name)(##args);      \
            }                                                                  \
        }                                                                      \
} while (false)

#define OBJECT(x, member)                                                      \
        ((x)->CC_CONCAT(, member))

#define OBJECT_COMPONENT(x, component)                                         \
        ((x)->CC_CONCAT(, component))

#define OBJECT_PUBLIC_DATA(x, member)                                          \
        ((x)->data.CC_CONCAT(m_, member))

#define OBJECT_CALL_PUBLIC_MEMBER(x, name, args...)                            \
        ((x))->functions.CC_CONCAT(m_, name)((struct object *)(x), ##args)

struct object {
        OBJECT_DECLARATIONS
} __may_alias;

extern void object_component_init(const struct object *);
extern void object_component_update(const struct object *);
extern void object_component_draw(const struct object *);

#endif /* !ENGINE_OBJECT_H */
