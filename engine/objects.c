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
        uint32_t oc_depth;
        struct object *oc_parent;
        struct object *oc_object;
        fix16_vector3_t oc_position;
        struct object_contexts oc_children;

        TAILQ_ENTRY(object_context) oc_entries;
} __aligned(32);

static bool _initialized = false;
struct object_context _root_ctx;

/* Caching */
/* Cached list of objects allocated */
static bool _cached_objects_dirty = true;
static struct objects _cached_objects[OBJECTS_MAX];

static bool _cached_sorted_objects_dirty = true;
static struct objects _cached_sorted_objects[OBJECTS_MAX];

/* Cached list of objects last searched using objects_component_find() */
static struct object *_cached_objects_component[OBJECTS_MAX];

/* Cached pointer to camera component */
static const struct camera *_cached_camera = NULL;

MEMB(_object_context_pool, struct object_context, OBJECTS_MAX,
    sizeof(struct object_context));

static void traverse_object_context_add(struct object_context *,
    struct object *);
static void traverse_object_context_remove(struct object_context *,
    struct object *);
static void traverse_object_context_update(struct object_context *);

/*
 * Initialize objects system.
 */
void
objects_init(void)
{
        static struct object root = {
                .id = 0,
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

        uint32_t object_idx;
        for (object_idx = 0; object_idx < OBJECTS_MAX; object_idx++) {
                _cached_objects[object_idx].object = NULL;
                _cached_sorted_objects[object_idx].object = NULL;
        }

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
        _cached_sorted_objects_dirty = true;
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
        _cached_sorted_objects_dirty = true;
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

        struct object *parent;
        parent = child_ctx->oc_parent;

        struct object_context *parent_ctx;
        parent_ctx = (struct object_context *)parent->context;

        traverse_object_context_remove(parent_ctx, child);

        _cached_objects_dirty = true;
        _cached_sorted_objects_dirty = true;
}

void
objects_clear(void)
{
        assert(_initialized);

        /* Clear cached */
        _cached_camera = NULL;

        _cached_objects[0].object = NULL;
        _cached_sorted_objects[0].object = NULL;

        _cached_objects_component[0] = NULL;

        struct object_context *itr_child_context;
        TAILQ_FOREACH (itr_child_context, &_root_ctx.oc_children,
            oc_entries) {
                struct object *itr_child;
                itr_child = itr_child_context->oc_object;

                traverse_object_context_remove(&_root_ctx,
                    itr_child);
        }

        _cached_objects_dirty = true;
        _cached_sorted_objects_dirty = true;
}

const struct objects *
objects_list(void)
{
        assert(_initialized);

        traverse_object_context_update(&_root_ctx);

        if (_cached_objects_dirty) {
                goto return_list;
        }

        /* uint32_t object_count; */
        /* object_count = traverse_object_context_populate(&_root_ctx, */
        /*     _cached_objects); */
        /* _cached_objects[object_count].object = NULL; */

        _cached_objects_dirty = false;

return_list:
        return (const struct objects *)&_cached_objects[0];
}

const struct objects *
objects_sorted_list(void)
{
        assert(_initialized);

        traverse_object_context_update(&_root_ctx);

        if (!_cached_sorted_objects_dirty) {
                goto return_list;
        }

        _cached_sorted_objects_dirty = false;

return_list:
        return (const struct objects *)&_cached_sorted_objects[0];
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
        TAILQ_INIT(&child_ctx->oc_children);

        TAILQ_INSERT_TAIL(&parent_ctx->oc_children, child_ctx,
            oc_entries);

        /* Connect context to child object */
        child->context = child_ctx;
}

/*
 * Remove object and its children from the tree.
 */
static void
traverse_object_context_remove(struct object_context *parent_ctx,
    struct object *remove)
{
        assert(parent_ctx != NULL);
        assert(remove != NULL);

        struct object_context *remove_ctx;
        remove_ctx = (struct object_context *)remove->context;

        /* Remove context from object */
        remove->context = NULL;
        TAILQ_REMOVE(&parent_ctx->oc_children, remove_ctx, oc_entries);

        struct object_context *itr_child_ctx;
        TAILQ_FOREACH (itr_child_ctx, &remove_ctx->oc_children,
            oc_entries) {
                struct object *itr_object;
                itr_object = itr_child_ctx->oc_object;
                traverse_object_context_remove(remove_ctx, itr_object);
        }

        /* Free context */
        assert((memb_free(&_object_context_pool, remove_ctx)) == 0);
}

static void
traverse_object_context_update(struct object_context *object_ctx)
{
        assert(object_ctx != NULL);

        struct object *parent;
        parent = object_ctx->oc_parent;

        struct object_context *parent_ctx;
        if (parent != NULL) {
                parent_ctx = (struct object_context *)parent->context;

                /* Update the relative position */
                fix16_vector3_add(&object_ctx->oc_position,
                    &parent_ctx->oc_position,
                    &object_ctx->oc_position);
        }

        struct object_context *itr_child_ctx;
        TAILQ_FOREACH (itr_child_ctx, &object_ctx->oc_children, oc_entries) {
                traverse_object_context_update(itr_child_ctx);
        }
}
