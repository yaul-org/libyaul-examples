/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdbool.h>
#include <stdio.h>

#include "objects.h"
#include "object.h"

#define COMPONENT_EVENT(x, name, args...) do {                                 \
        if (((struct component *)(x))->CC_CONCAT(on_, name) != NULL) {         \
                if (COMPONENT(((struct component *)(x)), active)) {            \
                        assert(COMPONENT((struct component *)(x), initialized)); \
                        ((struct component *)(x))->CC_CONCAT(on_, name)(       \
                                (struct component *)(x), ##args);              \
                }                                                              \
        }                                                                      \
} while (false)

#define COMPONENT_UPDATE(x)     COMPONENT_EVENT(x, update)
#define COMPONENT_DRAW(x)       COMPONENT_EVENT(x, draw)
#define COMPONENT_DESTROY(x)    COMPONENT_EVENT(x, destroy)

void
object_init(struct object *object)
{
        assert(object != NULL);
        assert(object->context == NULL);

        /* Register object (initialize its context) */
        objects_object_register(object);

        /* Make sure first component is the transform */
        assert((OBJECT_COMPONENT(object, COMPONENT_ID_TRANSFORM) != NULL) &&
               (OBJECT_COMPONENT(object, COMPONENT_ID_TRANSFORM)->id == COMPONENT_ID_TRANSFORM));

        uint32_t component_idx;
        for (component_idx = 1;
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);

                assert(!COMPONENT(component, initialized));

                assert(component->on_init != NULL);
                component->on_init(component);

                /* Mark component Initialized */
                COMPONENT(component, initialized) = true;
        }
}

void
object_destroy(struct object *object)
{
        assert(object != NULL);
}

void
object_update(const struct object *object)
{
        assert(object != NULL);

        uint32_t component_idx;
        for (component_idx = 0;
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT(object, component_list)[component_idx];

                COMPONENT_UPDATE(component);
        }
}

void
object_draw(const struct object *object)
{
        assert(object != NULL);

        uint32_t component_idx;
        for (component_idx = 0;
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT(object, component_list)[component_idx];

                COMPONENT_DRAW(component);
        }
}
