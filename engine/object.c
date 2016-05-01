/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdbool.h>
#include <stdio.h>

#include "object.h"

void
object_component_init(const struct object *object)
{
        assert(object != NULL);

        uint32_t component_idx;
        for (component_idx = 0;
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT(object, component_list)[component_idx];

                COMPONENT_INIT(component);
        }
}

void
object_component_update(const struct object *object)
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
object_component_draw(const struct object *object)
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
