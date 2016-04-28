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

#define OBJECTS_DEPTH_MAX 3

struct object_context;

TAILQ_HEAD(object_contexts, object_context);

struct object_context {
        uint32_t oc_depth;
        struct object *oc_parent;
        struct object *oc_object;
        struct object_contexts oc_children;

        TAILQ_ENTRY(object_context) oc_entries;
} __aligned(32);

static bool _initialized = false;
struct object_context _root_context;

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
static uint32_t traverse_object_context_populate(struct object_context *,
    struct objects *);

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
                .context = &_root_context
        };

        if (_initialized) {
                return;
        }

        /* Initialize root context */
        _root_context.oc_depth = 1;
        _root_context.oc_parent = NULL;
        _root_context.oc_object = &root;
        TAILQ_INIT(&_root_context.oc_children);

        memb_init(&_object_context_pool);

        uint32_t object_idx;
        for (object_idx = 0; object_idx < OBJECTS_MAX; object_idx++) {
                _cached_objects[object_idx].parent = NULL;
                _cached_objects[object_idx].object = NULL;

                _cached_sorted_objects[object_idx].parent = NULL;
                _cached_sorted_objects[object_idx].object = NULL;
        }

        _initialized = true;
}

/*
 * Add child object to root object.
 */
void
objects_object_add(struct object *child_object)
{
        assert(_initialized);
        assert(child_object != NULL);

        /* Caching */
        if (child_object->camera != NULL) {
                _cached_camera = child_object->camera;
        }

        traverse_object_context_add(&_root_context, child_object);

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
objects_object_child_add(struct object *object, struct object *child_object)
{
        assert(_initialized);
        assert(object != NULL);
        assert(child_object != NULL);
        assert(object->context != NULL);

        traverse_object_context_add(object->context, child_object);

        _cached_objects_dirty = true;
        _cached_sorted_objects_dirty = true;
}

/*
 * Remove object and its children.
 */
void
objects_object_remove(struct object *child_object)
{
        assert(_initialized);
        assert(child_object != NULL);
        assert(child_object->context != NULL);

        struct object_context *child_ctx;
        child_ctx = child_object->context;

        struct object *parent;
        parent = child_ctx->oc_parent;

        struct object_context *parent_ctx;
        parent_ctx = parent->context;

        traverse_object_context_remove(parent_ctx, child_object);

        _cached_objects_dirty = true;
        _cached_sorted_objects_dirty = true;
}

void
objects_clear(void)
{
        assert(_initialized);

        /* Clear cached */
        _cached_camera = NULL;

        _cached_objects[0].parent = NULL;
        _cached_objects[0].object = NULL;

        _cached_sorted_objects[0].parent = NULL;
        _cached_sorted_objects[0].object = NULL;

        _cached_objects_component[0] = NULL;

        struct object_context *itr_child_object_context;
        TAILQ_FOREACH (itr_child_object_context, &_root_context.oc_children,
            oc_entries) {
                struct object *itr_child_object;
                itr_child_object = itr_child_object_context->oc_object;

                traverse_object_context_remove(&_root_context,
                    itr_child_object);
        }

        _cached_objects_dirty = true;
        _cached_sorted_objects_dirty = true;
}

const struct objects *
objects_list(void)
{
        assert(_initialized);

        if (_cached_objects_dirty) {
                uint32_t object_count;
                object_count = traverse_object_context_populate(&_root_context,
                    _cached_objects);

                _cached_objects[object_count].parent = NULL;
                _cached_objects[object_count].object = NULL;
        }

        _cached_objects_dirty = false;

        return (const struct objects *)&_cached_objects[0];
}

const struct objects *
objects_sorted_list(void)
{
        assert(_initialized);

        if (!_cached_sorted_objects_dirty) {
                goto return_list;
        }

        /* We know that if the cached sorted object list is dirty, the
         * cached object must also be dirty as well. */
        (void)objects_list();

        uint32_t sorted_object_idx;
        sorted_object_idx = 0;

        int32_t z_value;
        for (z_value = OBJECTS_Z_MAX; z_value > 0; z_value--) {
                uint32_t object_idx;
                for (object_idx = 0; _cached_objects[object_idx].object != NULL;
                     object_idx++) {
                        const struct object *parent;
                        parent = _cached_objects[object_idx].parent;

                        const struct object *object;
                        object = _cached_objects[object_idx].object;

                        int32_t transform_z;
                        transform_z = fix16_to_int(OBJECT_COMPONENT(
                                    object, transform).position.z);

                        if (transform_z == z_value) {
                                _cached_sorted_objects[sorted_object_idx].parent = parent;
                                _cached_sorted_objects[sorted_object_idx].object = object;
                                sorted_object_idx++;
                        }
                }
        }

        _cached_sorted_objects[sorted_object_idx].parent = NULL;
        _cached_sorted_objects[sorted_object_idx].object = NULL;

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
    struct object *child_object)
{
        assert(child_object != NULL);
        assert(parent_ctx != NULL);
        assert(child_object->context == NULL);

        struct object *parent;
        parent = parent_ctx->oc_object;

        /* Allocate child object */
        struct object_context *child_object_ctx;
        child_object_ctx = (struct object_context *)memb_alloc(
                &_object_context_pool);
        assert(child_object_ctx != NULL);

        assert((child_object_ctx->oc_depth + 1) <= OBJECTS_DEPTH_MAX);

        child_object_ctx->oc_depth = parent_ctx->oc_depth + 1;
        child_object_ctx->oc_parent = parent;
        child_object_ctx->oc_object = child_object;
        TAILQ_INIT(&child_object_ctx->oc_children);

        TAILQ_INSERT_TAIL(&parent_ctx->oc_children, child_object_ctx,
            oc_entries);

        /* Connect context to child object */
        child_object->context = child_object_ctx;
}

/*
 * Remove object and its children from the tree.
 */
static void
traverse_object_context_remove(struct object_context *parent_ctx,
    struct object *remove)
{
        static uint32_t depth = 1;

        assert(parent_ctx != NULL);
        assert(remove != NULL);

        assert(depth <= OBJECTS_DEPTH_MAX);

        struct object_context *remove_ctx;
        remove_ctx = remove->context;

        /* Remove context from object */
        remove->context = NULL;
        TAILQ_REMOVE(&parent_ctx->oc_children, remove_ctx, oc_entries);

        if ((TAILQ_EMPTY(&remove_ctx->oc_children))) {
                goto exit;
        }

        depth++;

        struct object_context *itr_child_object_ctx;
        TAILQ_FOREACH (itr_child_object_ctx, &remove_ctx->oc_children,
            oc_entries) {
                struct object *itr_object;
                itr_object = itr_child_object_ctx->oc_object;
                traverse_object_context_remove(remove_ctx, itr_object);
        }

        depth--;

exit:
        /* Free context */
        assert((memb_free(&_object_context_pool, remove_ctx)) == 0);
}

/*
 * Traverse the tree and populate an array of pointers to objects.
 *
 * The number of objects inserted in the array of pointers to objects is
 * returned.
 */
static uint32_t __unused
traverse_object_context_populate(struct object_context *object_ctx,
    struct objects *objects)
{
        static uint32_t depth = 1;

        assert(object_ctx != NULL);
        assert(objects != NULL);
        assert(depth <= OBJECTS_DEPTH_MAX);

        /* Visit */
        objects[0].parent = object_ctx->oc_parent;
        objects[0].object = object_ctx->oc_object;

        if ((TAILQ_EMPTY(&object_ctx->oc_children))) {
                return 1;
        }

        depth++;

        uint32_t object_idx;
        object_idx = 0;

        struct object_context *itr_object_ctx;
        TAILQ_FOREACH (itr_object_ctx, &object_ctx->oc_children, oc_entries) {
                object_idx += traverse_object_context_populate(
                        itr_object_ctx, &objects[object_idx + 1]);
        }

        depth--;

        return 1 + object_idx;
}
