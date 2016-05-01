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

struct object_context;

TAILQ_HEAD(object_contexts, object_context);

struct object_context {
        /* Depth in objects tree */
        uint32_t oc_depth;
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
struct object_context _root_ctx;

/* Caching */
/* Cached list of objects allocated */
static bool _cached_objects_dirty = true;
static struct objects _cached_objects;
static struct object_z _cached_object_z[OBJECTS_MAX];
static struct object_z_entry _cached_object_z_entry[OBJECTS_MAX];

/* Cached pointer to camera component */
static const struct camera *_cached_camera = NULL;

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
        static struct object root = {
                .id = OBJECT_ID_RESERVED_0,
                .visible = false,
                .vertex_list = NULL,
                .vertex_count = 0,
                .color_list = NULL,
                .transform = {
                        .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 0.0f)
                },
                .camera = NULL,
                .colliders = NULL,
                .rigid_body = NULL,
                .component_list = {
                        NULL
                },
                .component_count = 0,
                .on_init = NULL,
                .on_update = NULL,
                .on_draw = NULL,
                .on_destroy = NULL,
                .on_collision = NULL,
                .on_trigger = NULL,
                .initialized = true,
                .context = &_root_ctx
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

/*
 * Add child object to root object.
 */
void
objects_object_add(struct object *child)
{
        assert(_initialized);
        assert(child != NULL);

        /* Caching */
        if (child->camera != NULL) {
                _cached_camera = child->camera;
        }

        traverse_object_context_add(&_root_ctx, child);

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
        assert(child != NULL);
        assert(object->context != NULL);

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

const struct object_z *
objects_list(void)
{
        assert(_initialized);

        traverse_object_context_update(&_root_ctx);

        return (const struct object_z *)&_cached_object_z[0];
}

const struct objects *
objects_sorted_list(void)
{
        assert(_initialized);

        traverse_object_context_update(&_root_ctx);

        return (const struct objects *)&_cached_objects;
}

const struct camera *
objects_component_camera_find(void)
{
        return _cached_camera;
}

static void
traverse_object_context_add(struct object_context *parent_ctx,
    struct object *child)
{
        assert(child != NULL);
        assert(parent_ctx != NULL);
        assert(child->context == NULL);

        struct object *parent;
        parent = parent_ctx->oc_object;

        /* Allocate child object */
        struct object_context *child_ctx;
        child_ctx = (struct object_context *)memb_alloc(
                &_object_context_pool);
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
 *
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
                        transform = &OBJECT_COMPONENT(object, transform);

                        /* Calculate the absolute position */
                        fix16_vector3_add(&COMPONENT(transform, position),
                            &parent_ctx->oc_position,
                            &top_object_ctx->oc_position);

                        /* Insert object into its corresponding Z "bucket" */
                        uint32_t z_position;
                        z_position = (uint32_t)fix16_to_int(
                                top_object_ctx->oc_position.z);

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

                struct object_context *itr_child_ctx;
                TAILQ_FOREACH_REVERSE (itr_child_ctx,
                    &top_object_ctx->oc_children,
                    object_contexts,
                    oc_tq_entries) {
                        SLIST_INSERT_HEAD(&stack, itr_child_ctx, oc_sl_entries);
                }
        }
}
