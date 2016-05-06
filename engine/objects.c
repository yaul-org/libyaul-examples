/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "objects.h"

#define OBJECTS_DEPTH_MAX 3

extern uint32_t tick;

struct object_context;

TAILQ_HEAD(object_contexts, object_context);

struct object_context {
        /* Depth in objects tree */
        int32_t oc_depth;
        /* Immediate parent */
        struct object *oc_parent;
        struct object *oc_object;
        /* Calculated absolute position of the object in world space */
        fix16_vector3_t oc_position;
        struct object_contexts oc_children;

        /* Tree management */
        TAILQ_ENTRY(object_context) oc_tq_entries;
        /* For non-recursive traversal */
        SLIST_ENTRY(object_context) oc_sl_entries;
} __aligned(32);

static bool _initialized = false;
/* Last tick when objects data was requested via objects_list() or
 * objects_sorted_list() */
static uint32_t _last_tick = 0;
static struct object_context _root_ctx;

/* Caching */
/* Cached list of objects allocated */
static bool _cached_objects_dirty = true;
static struct objects _cached_objects;
static struct object_z _cached_object_z[OBJECTS_MAX];
static struct object_z_entry _cached_object_z_entry[OBJECTS_MAX];

/* Cached pointer to camera component */
static const struct component *_cached_camera = NULL;

MEMB(_object_context_pool, struct object_context, OBJECTS_MAX,
    sizeof(struct object_context));

static void traverse_object_context_add(struct object_context *,
    struct object *);
static void traverse_object_context_remove(struct object_context *);
static void traverse_object_context_update(struct object_context *);

/*
 * Initialize objects system.
 */
void
objects_init(void)
{
        static struct transform transform = {
                .active = true,
                .id = COMPONENT_ID_TRANSFORM,
                .object = NULL,
                .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 0.0f)
        };

        /* "Null" object */
        static struct object root = {
                .active = true,
                .id = OBJECT_ID_RESERVED_BEGIN + 0x0000,
                .component_list = {
                        (struct component *)&transform
                },
                .component_count = 1,
                .on_destroy = NULL,
        };

        if (_initialized) {
                return;
        }

        /* Initialize root context */
        _root_ctx.oc_depth = 1;
        _root_ctx.oc_parent = NULL;
        _root_ctx.oc_object = &root;
        fix16_vector3_zero(&_root_ctx.oc_position);
        TAILQ_INIT(&_root_ctx.oc_children);

        memb_init(&_object_context_pool);

        _initialized = true;
}

void
objects_object_register(struct object *object)
{
        assert(object != NULL);
        assert(object->context == NULL);

        /* Initialize context */
        struct object_context *object_ctx;
        object_ctx = (struct object_context *)memb_alloc(&_object_context_pool);
        assert(object_ctx != NULL);

        object_ctx->oc_depth = -1;
        object_ctx->oc_parent = NULL;
        object_ctx->oc_object = object;

        fix16_vector3_zero(&object_ctx->oc_position);

        TAILQ_INIT(&object_ctx->oc_children);

        /* Connect context to object object */
        object->context = object_ctx;
}

void
objects_object_unregister(struct object *object)
{
        assert(object != NULL);
        assert(object->context != NULL);
}

/*
 * Determine if the object is already present in the objects tree.
 */
bool
objects_object_added(const struct object *object)
{
        assert(object != NULL);

        struct object_context *object_ctx;
        object_ctx = (struct object_context *)object->context;

        return (object_ctx != NULL) && (object_ctx->oc_object == object);
}

/*
 * Add child object to root object.
 */
void
objects_object_add(struct object *object)
{
        assert(_initialized);
        assert(object != NULL);
        /* Has the object been registered? */
        assert(object->context != NULL);

        traverse_object_context_add(&_root_ctx, object);

        _cached_objects_dirty = true;
}

/*
 * Add child object to parent object.
 *
 * Note that there is no check that a back, forward, or cross edges (a
 * cycle) is introduced when adding a child object. When traversing the
 * tree, an infinite loop will occur should a cycle be present.
 */
void
objects_object_child_add(struct object *object, struct object *child)
{
        assert(_initialized);
        assert(object != NULL);
        assert(object->context != NULL);
        assert(child != NULL);
        assert(child->context == NULL);

        traverse_object_context_add((struct object_context *)object->context,
            child);

        _cached_objects_dirty = true;
}

/*
 * Remove object and its children.
 */
void
objects_object_remove(struct object *child)
{
        assert(_initialized);
        assert(child != NULL);
        assert(child->context != NULL);

        struct object_context *child_ctx;
        child_ctx = (struct object_context *)child->context;

        traverse_object_context_remove(child_ctx);

        _cached_objects_dirty = true;
}

void
objects_clear(void)
{
        assert(_initialized);

        /* Clear cached */
        {
                _cached_camera = NULL;

                _cached_object_z[0].object = NULL;

                /* Empty each bucket */
                uint32_t bucket_idx;
                for (bucket_idx = 0; bucket_idx < OBJECTS_Z_MAX_BUCKETS;
                     bucket_idx++) {
                        STAILQ_INIT(&_cached_objects.buckets[bucket_idx]);
                }
        }

        while (!(TAILQ_EMPTY(&_root_ctx.oc_children))) {
                struct object_context *itr_ctx;
                itr_ctx = TAILQ_FIRST(&_root_ctx.oc_children);

                struct object *itr_child;
                itr_child = itr_ctx->oc_object;

                objects_object_remove(itr_child);
        }

        _cached_objects_dirty = true;
}

/*
 * Return an array of objects in pre-order traversal from the objects
 * tree.
 */
const struct object_z *
objects_list(void)
{
        assert(_initialized);

        bool new_frame;
        new_frame = (tick - _last_tick) != 0;

        if (!new_frame && !_cached_objects_dirty) {
                goto return_list;
        }

        traverse_object_context_update(&_root_ctx);

        _cached_objects_dirty = false;

return_list:
        _last_tick = tick;

        return (const struct object_z *)&_cached_object_z[0];
}

const struct objects *
objects_sorted_list(void)
{
        assert(_initialized);

        bool new_frame;
        new_frame = (tick - _last_tick) != 0;

        if (!new_frame && !_cached_objects_dirty) {
                goto return_list;
        }

        traverse_object_context_update(&_root_ctx);

        _cached_objects_dirty = false;

return_list:
        _last_tick = tick;

        return (const struct objects *)&_cached_objects;
}

const struct component *
objects_component_find(int32_t component_id)
{
        assert((component_id >= 0) &&
               (component_id < OBJECT_COMPONENT_LIST_MAX));

        /* Caching */
        if ((component_id == COMPONENT_ID_CAMERA) && (_cached_camera != NULL)) {
                return _cached_camera;
        }

        const struct object_z *objects;
        objects = objects_list();

        uint32_t object_idx;
        for (object_idx = 0; objects[object_idx].object != NULL; object_idx++) {
                const struct object *object;
                object = objects[object_idx].object;

                const struct component *component;
                component = objects_object_component_find(object, component_id);
                if (component != NULL) {
                        return component;
                }
        }

        return NULL;
}

const struct component *
objects_object_component_find(const struct object *object, int32_t component_id)
{
        assert(object != NULL);
        assert(object->context != NULL);
        assert((component_id >= 0) &&
               (component_id < OBJECT_COMPONENT_LIST_MAX));

        /* Caching */
        if (component_id == COMPONENT_ID_TRANSFORM) {
                return OBJECT_COMPONENT(object, COMPONENT_ID_TRANSFORM);
        }

        if ((component_id == COMPONENT_ID_CAMERA) && (_cached_camera != NULL)) {
                return _cached_camera;
        }

        uint32_t component_idx;
        for (component_idx = 0;
             component_idx < OBJECT(object, component_count);
             component_idx++) {
                struct component *component;
                component = OBJECT_COMPONENT(object, component_idx);

                if (COMPONENT(component, id) == component_id) {
                        assert(COMPONENT(component, initialized));

                        /* Caching */
                        if (component_id == COMPONENT_ID_CAMERA) {
                                _cached_camera = component;
                        }

                        return component;
                }
        }

        return NULL;
}

static void
traverse_object_context_add(struct object_context *parent_ctx,
    struct object *child)
{
        assert(child != NULL);
        assert(parent_ctx != NULL);
        assert(child->context != NULL);

        struct object *parent;
        parent = parent_ctx->oc_object;

        /* Allocate child object */
        struct object_context *child_ctx;
        child_ctx = (struct object_context *)memb_alloc(&_object_context_pool);
        assert(child_ctx != NULL);

        assert((child_ctx->oc_depth + 1) <= OBJECTS_DEPTH_MAX);

        child_ctx->oc_depth = parent_ctx->oc_depth + 1;
        child_ctx->oc_parent = parent;
        child_ctx->oc_object = child;
        fix16_vector3_zero(&child_ctx->oc_position);
        TAILQ_INIT(&child_ctx->oc_children);

        TAILQ_INSERT_TAIL(&parent_ctx->oc_children, child_ctx, oc_tq_entries);

        /* Connect context to child object */
        child->context = child_ctx;
}

/*
 * Remove object and its children from the tree.
 */
static void
traverse_object_context_remove(struct object_context *remove_ctx)
{
        assert(remove_ctx != NULL);
        assert(remove_ctx->oc_object != NULL);
        assert(remove_ctx->oc_object->context = remove_ctx);

        SLIST_HEAD(stack, object_context) stack = SLIST_HEAD_INITIALIZER(stack);

        SLIST_INIT(&stack);

        SLIST_INSERT_HEAD(&stack, remove_ctx, oc_sl_entries);
        while (!(SLIST_EMPTY(&stack))) {
                struct object_context *top_remove_ctx;
                top_remove_ctx = SLIST_FIRST(&stack);

                SLIST_REMOVE_HEAD(&stack, oc_sl_entries);

                /* Remove context from object */
                const struct object *parent;
                parent = top_remove_ctx->oc_parent;
                /* The root node is not to be removed */
                if (parent != NULL) {
                        struct object_context *parent_ctx;
                        parent_ctx = (struct object_context *)parent->context;

                        struct object *top_remove;
                        top_remove = top_remove_ctx->oc_object;
                        /* Disconnect context from object */
                        top_remove->context = NULL;

                        top_remove_ctx->oc_depth = 0;
                        top_remove_ctx->oc_parent = NULL;
                        top_remove_ctx->oc_object = NULL;

                        TAILQ_REMOVE(&parent_ctx->oc_children, top_remove_ctx,
                            oc_tq_entries);

                        /* Free context */
                        assert((memb_free(&_object_context_pool,
                                    top_remove_ctx)) == 0);
                }

                while (!(TAILQ_EMPTY(&top_remove_ctx->oc_children))) {
                        struct object_context *itr_child_ctx;
                        itr_child_ctx =
                            TAILQ_FIRST(&top_remove_ctx->oc_children);

                        SLIST_INSERT_HEAD(&stack, itr_child_ctx, oc_sl_entries);
                }
        }
}

/*
 * Traverse objects tree to update each object's absolute position and
 * populate the Z buckets.
 */
static void
traverse_object_context_update(struct object_context *object_ctx)
{
        assert(object_ctx != NULL);
        assert(object_ctx->oc_object->context = object_ctx);

        /* Empty each bucket */
        uint32_t bucket_idx;
        for (bucket_idx = 0; bucket_idx < OBJECTS_Z_MAX_BUCKETS; bucket_idx++) {
                STAILQ_INIT(&_cached_objects.buckets[bucket_idx]);
        }

        uint32_t obj_z_idx;
        obj_z_idx = 0;

        _cached_object_z[obj_z_idx].object = NULL;

        uint32_t objs_z_idx;
        objs_z_idx = 0;

        SLIST_HEAD(stack, object_context) stack = SLIST_HEAD_INITIALIZER(stack);

        SLIST_INIT(&stack);

        SLIST_INSERT_HEAD(&stack, object_ctx, oc_sl_entries);
        while (!(SLIST_EMPTY(&stack))) {
                struct object_context *top_object_ctx;
                top_object_ctx = SLIST_FIRST(&stack);

                SLIST_REMOVE_HEAD(&stack, oc_sl_entries);

                const struct object *parent;
                parent = top_object_ctx->oc_parent;
                if (parent != NULL) {
                        const struct object_context *parent_ctx;
                        parent_ctx = parent->context;

                        struct object *object;
                        object = top_object_ctx->oc_object;

                        /* Ensure that the context is connected to the
                         * right object */
                        assert(top_object_ctx == object->context);

                        struct transform *transform;
                        transform = (struct transform *)OBJECT_COMPONENT(object,
                            COMPONENT_ID_TRANSFORM);

                        /* Calculate the absolute position */
                        fix16_vector3_add(&COMPONENT(transform, position),
                            &parent_ctx->oc_position,
                            &top_object_ctx->oc_position);

                        /* Insert object into its corresponding Z "bucket" */
                        int32_t z_position;
                        z_position = (int32_t)fix16_to_int(
                                top_object_ctx->oc_position.z);

                        /* Make sure the Z position falls within the buckets */
                        assert((z_position >= OBJECTS_Z_MIN) &&
                               (z_position <= OBJECTS_Z_MAX));

                        struct object_z *object_z;
                        object_z = &_cached_object_z[obj_z_idx];
                        obj_z_idx++;

                        object_z->object = object;
                        object_z->position = &top_object_ctx->oc_position;

                        struct object_z_entry *object_z_entry;
                        object_z_entry = &_cached_object_z_entry[objs_z_idx];
                        objs_z_idx++;

                        object_z_entry->object_z = object_z;

                        STAILQ_INSERT_TAIL(&_cached_objects.buckets[z_position],
                            object_z_entry, entries);
                }

                struct object_contexts *children;
                children = &top_object_ctx->oc_children;
                struct object_context *itr_child_ctx;
                TAILQ_FOREACH_REVERSE (itr_child_ctx, children, object_contexts,
                    oc_tq_entries) {
                        SLIST_INSERT_HEAD(&stack, itr_child_ctx, oc_sl_entries);
                }
        }
}
