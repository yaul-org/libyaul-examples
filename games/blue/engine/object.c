/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "engine.h"

#define COMPONENT_EVENT_CALL(x, name, args...) do {                            \
        if (COMPONENT_EVENT(((struct component *)(x)), name) != NULL) {        \
                if (COMPONENT(((struct component *)(x)), active)) {            \
                        assert(COMPONENT((struct component *)(x), initialized)); \
                        COMPONENT_EVENT(x, name)((struct component *)(x),      \
                            ##args);                                           \
                }                                                              \
        }                                                                      \
} while (false)

#define COMPONENT_UPDATE(x)     COMPONENT_EVENT_CALL(x, update)
#define COMPONENT_DRAW(x)       COMPONENT_EVENT_CALL(x, draw)

void
object_init(struct object *object)
{
        assert(object != NULL);

        /* Register object (initialize its context) */
        objects_object_register(object);

        /* Make sure we have at least one component (transform) */
        assert(OBJECT(object, component_count) >= 1);

        /* Make sure first component is the transform */
        const struct transform *transform __unused;
        transform = (const struct transform *)OBJECT_COMPONENT(object, 0);
        assert((transform != NULL) &&
               (COMPONENT(transform, id) == COMPONENT_ID_TRANSFORM));

        uint32_t component_idx;
        for (component_idx = 1; /* Skip transform component */
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);

                object_component_init(object, component);
        }
}

void
object_destroy(struct object *object)
{
        assert(object != NULL);

        /* The object has to be removed from the objects tree before
         * destroying the object */
        assert(!(objects_object_added(object)));

        /* Destroy components */
        uint32_t component_idx;
        for (component_idx = 0; component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);
                assert(component == NULL);

                /* Destroy */
                /* Must already have been initialized */
                assert(COMPONENT(component, initialized));
                /* Destroy event must be present */
                assert(COMPONENT_EVENT(component, destroy) != NULL);
                COMPONENT_EVENT(component, destroy)(component);
                /* Uninitialize the component */
                COMPONENT(component, initialized) = false;

                if (OBJECT(object, context).instantiated) {
                        /* Free components */
                        free(component);

                        OBJECT_COMPONENT(object, component_idx) = NULL;
                        OBJECT_COMPONENT_SIZE(object, component_idx) = 0;
                }
        }
        OBJECT(object, component_count) = 0;

        OBJECT(object, context).instantiated = false;

        OBJECT(object, active) = false;
        OBJECT(object, id) = 0;

        /* Remove from tree */
        if (objects_object_added(object)) {
                objects_object_remove(object);
        }

        /* Unregister object */
        objects_object_unregister(object);
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
             component_idx < OBJECT_COMPONENT_LIST_MAX;
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);

                if (component == NULL) {
                        continue;
                }

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
             component_idx < OBJECT_COMPONENT_LIST_MAX;
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);

                if (component == NULL) {
                        continue;
                }

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
 *     to instantiate from a prefab (const) source object.
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
        for (component_idx = 0;
             component_idx < OBJECT_COMPONENT_LIST_MAX;
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);
                if (component == NULL) {
                        continue;
                }

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

        /* Mark object as an instantiated object */
        OBJECT(copy, context).instantiated = true;

        object_init(copy);
}

/*
 *
 */
void
object_component_init(const struct object *object, struct component *component)
{
        assert(object != NULL);
        assert(component != NULL);
        /* Component ID must be within range */
        assert((COMPONENT(component, id) > 0) &&
               (((COMPONENT(component, id) & COMPONENT_ID_BUILTIN_MASK) == 0) ||
                ((COMPONENT(component, id) & COMPONENT_ID_USER_MASK) == 0)));
        /* Must be active */
        assert(COMPONENT(component, active));
        /* Must not already have been initialized */
        assert(!COMPONENT(component, initialized));
        /* Init event must be present */
        assert(COMPONENT_EVENT(component, init) != NULL);

        /* Initialize */
        COMPONENT(component, object) = object;
        COMPONENT_EVENT(component, init)(component);
        COMPONENT(component, initialized) = true;
}

/*
 *
 */
void
object_component_add(struct object *object, struct component *component,
        uint32_t component_size)
{
        assert(object != NULL);
        assert(component != NULL);
        assert(component_size >= sizeof(struct component));

        assert(OBJECT(object, component_count) <= OBJECT_COMPONENT_LIST_MAX);

        /* Don't add transform component */
        assert(COMPONENT(component, id) != COMPONENT_ID_TRANSFORM);

        uint32_t component_idx;
        for (component_idx = 0; component_idx < OBJECT_COMPONENT_LIST_MAX;
             component_idx++) {
                if (OBJECT_COMPONENT(object, component_idx) != NULL) {
                        continue;
                }

                OBJECT_COMPONENT(object, component_idx) = component;
                OBJECT_COMPONENT_SIZE(object, component_idx) = component_size;

                OBJECT(object, component_count)++;
                return;
        }

        assert(false && "Component list full");
}

/*
 *
 */
void
object_component_remove(struct object *object,
    const struct component *component)
{
        assert(object != NULL);
        assert(component != NULL);

        assert(OBJECT(object, component_count) > 0);

        /* Don't remove transform component */
        assert(COMPONENT(component, id) != COMPONENT_ID_TRANSFORM);

        uint32_t component_idx;
        for (component_idx = 0; component_idx < OBJECT_COMPONENT_LIST_MAX;
             component_idx++) {
                if (OBJECT_COMPONENT(object, component_idx) != component) {
                        continue;
                }

                OBJECT_COMPONENT(object, component_idx) = NULL;
                OBJECT_COMPONENT_SIZE(object, component_idx) = 0;
                OBJECT(object, component_count)--;
                return;
        }

        assert(false && "Component not found");
}

/*
 *
 */
const struct component *
object_component_find(const struct object *object, int32_t component_id)
{
        assert(object != NULL);
        assert((component_id > 0) &&
               (((component_id & COMPONENT_ID_BUILTIN_MASK) == 0) ||
                ((component_id & COMPONENT_ID_USER_MASK) == 0)));

        /* Caching */
        if (component_id == COMPONENT_ID_TRANSFORM) {
                return OBJECT_COMPONENT(object, 0);
        }

        uint32_t component_idx;
        for (component_idx = 0;
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);

                if (COMPONENT(component, id) == component_id) {
                        assert(COMPONENT(component, initialized));

                        return component;
                }
        }

        return NULL;
}

/*
 *
 */
void
object_component_find_all(const struct object *object, int32_t component_id,
    struct component **component_list)
{
        assert(object != NULL);
        assert(OBJECT(object, component_count) <= OBJECT_COMPONENT_LIST_MAX);
        assert((component_id > 0) &&
               (((component_id & COMPONENT_ID_BUILTIN_MASK) == 0) ||
                ((component_id & COMPONENT_ID_USER_MASK) == 0)));

        /* Caching */
        if (component_id == COMPONENT_ID_TRANSFORM) {
                component_list[0] = OBJECT_COMPONENT(object, 0);
                component_list[1] = NULL;
                return;
        }

        uint32_t component_list_idx;
        component_list_idx = 0;

        uint32_t component_idx;
        for (component_idx = 0;
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);

                if (COMPONENT(component, id) == component_id) {
                        assert(COMPONENT(component, initialized));

                        component_list[component_list_idx] = component;
                        component_list_idx++;
                }
        }

        component_list[component_list_idx] = NULL;
}
