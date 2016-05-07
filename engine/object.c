/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

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

        if (!OBJECT(object, active)) {
                return;
        }

        uint32_t component_idx;
        for (component_idx = 0;
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);

                COMPONENT_UPDATE(component);
        }
}

void
object_draw(const struct object *object)
{
        assert(object != NULL);

        if (!OBJECT(object, active)) {
                return;
        }

        uint32_t component_idx;
        for (component_idx = 0;
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);

                COMPONENT_DRAW(component);
        }
}

/*
 * Create and initialize a copy of an object from a source object.
 *
 * Note the following restrictions:
 *   - The copied object is initially deactivated and must be activated.
 *   - All children objects of the source object will not be copied.
 *   - Copied object will be instantiated and initialized. It is preferred
 *     to instantiate from a prefab source object.
 *   - All components copied must explicitly freed by the caller.
 */
void
object_instantiate(const struct object *object, struct object *copy,
    uint32_t object_size)
{
        assert(object != NULL);
        assert(copy != NULL);
        /* The size of the object cannot be zero or less than the size
         * of base object */
        assert(((int32_t)object_size - (int32_t)sizeof(struct object)) >= 0);

        /* Clear base object */
        memset(copy, 0x00, sizeof(struct object));

        OBJECT(copy, active) = false;
        OBJECT(copy, id) = OBJECT(object, id);

        /* Copy the rest of the data (if there is any) */
        void *inherited_copy;
        inherited_copy = (void *)((uintptr_t)copy + sizeof(struct object));
        void *inherited_object;
        inherited_object = (void *)((uintptr_t)object + sizeof(struct object));
        (void)memcpy(inherited_copy, inherited_object,
            object_size - sizeof(struct object));

        /* Allocate components */
        uint32_t component_idx;
        for (component_idx = 0; component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);
                assert(component != NULL);

                uint32_t component_size;
                component_size = OBJECT_COMPONENT_SIZE(object, component_idx);
                assert(component_size != 0);

                struct component *copy_component;
                copy_component = (struct component *)malloc(component_size);
                assert(copy_component != NULL);

                (void)memcpy(copy_component, component, component_size);

                /* Have the component's pointer to object point to the
                 * new copy */
                COMPONENT(copy_component, object) = copy;
                COMPONENT(copy_component, initialized) = false;

                OBJECT_COMPONENT(copy, component_idx) = copy_component;
                OBJECT_COMPONENT_SIZE(copy, component_idx) = component_size;
        }

        OBJECT(copy, component_count) = OBJECT(object, component_count);
        OBJECT(copy, on_destroy) = OBJECT(object, on_destroy);

        object_init(copy);
}
