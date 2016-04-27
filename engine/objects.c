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

struct object_tree_node;

TAILQ_HEAD(object_tree, object_tree_node);

struct object_tree_node {
        uint32_t op_depth;
        const struct object *op_object;
        const struct transform *op_transform;
        struct object_tree op_children;

        TAILQ_ENTRY(object_tree_node) op_entries;
} __aligned(32);

static bool _initialized = false;
struct object_tree _object_tree;

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

MEMB(_object_tree_node_pool, struct object_tree_node, OBJECTS_MAX,
    sizeof(struct object_tree_node));

MEMB(_transform_pool, struct transform, OBJECTS_MAX,
    sizeof(struct transform));

static struct object_tree_node *traverse_object_tree_node_find(struct object_tree_node *,
    const struct object *);
static uint32_t traverse_object_tree_node_populate(struct object_tree_node *,
    struct objects *);
static void traverse_object_tree_node_remove(struct object_tree_node *,
    struct object_tree *);

/*
 * Initialize objects system.
 */
void
objects_init(void)
{
        if (_initialized) {
                return;
        }

        TAILQ_INIT(&_object_tree);

        memb_init(&_object_tree_node_pool);
        memb_init(&_transform_pool);

        uint32_t object_idx;
        for (object_idx = 0; object_idx < OBJECTS_MAX; object_idx++) {
                _cached_objects[object_idx].parent = NULL;
                _cached_objects[object_idx].object = NULL;
        }

        _initialized = true;
}

void
objects_object_add(const struct object *object)
{
        assert(_initialized);
        assert(object != NULL);

        struct object_tree_node *iter_object_tree_node;
        iter_object_tree_node = NULL;
        TAILQ_FOREACH (iter_object_tree_node, &_object_tree, op_entries) {
                const struct object *cur_object;
                cur_object = iter_object_tree_node->op_object;

                /* No duplicate objects can be added */
                assert(cur_object->id != object->id);
        }

        /* Caching */
        if (object->camera != NULL) {
                _cached_camera = object->camera;
        }

        struct object_tree_node *object_tree_node;
        object_tree_node = (struct object_tree_node *)memb_alloc(&_object_tree_node_pool);
        assert(object_tree_node != NULL);

        object_tree_node->op_object = object;
        object_tree_node->op_transform = NULL;
        TAILQ_INIT(&object_tree_node->op_children);
        object_tree_node->op_depth = 1;

        TAILQ_INSERT_TAIL(&_object_tree, object_tree_node, op_entries);

        _cached_objects_dirty = true;
        _cached_sorted_objects_dirty = true;
}

/*
 * Add child object to object.
 *
 * Note that there is no check that a back, forward, or cross edges (a
 * cycle) is introduced when adding a child object. When traversing the
 * tree, an infinite loop will occur should a cycle be present.
 */
void
objects_object_child_add(const struct object *object,
    const struct object *child_object)
{
        assert(_initialized);
        assert(object != NULL);
        assert(child_object != NULL);

        struct object_tree_node *object_tree_node;
        object_tree_node = NULL;

        struct object_tree_node *iter_object_tree_node;

        /* Look for the corresponding object */
        TAILQ_FOREACH (iter_object_tree_node, &_object_tree, op_entries) {
                const struct object *cur_object;
                cur_object = iter_object_tree_node->op_object;

                if (cur_object == object) {
                        object_tree_node = iter_object_tree_node;
                        break;
                }
        }
        assert(object_tree_node != NULL);

        /* Check for duplicates in object's children */
        TAILQ_FOREACH (iter_object_tree_node, &object_tree_node->op_children, op_entries) {
                const struct object *cur_object;
                cur_object = iter_object_tree_node->op_object;

                /* No duplicate objects can be added */
                assert(cur_object->id != object->id);
        }

        /* Allocate child object */
        struct object_tree_node *child_object_tree_node;
        child_object_tree_node = (struct object_tree_node *)memb_alloc(&_object_tree_node_pool);
        assert(child_object_tree_node != NULL);

        /* Assert we don't have a "tall" tree */
        assert((object_tree_node->op_depth + 1) <= OBJECTS_DEPTH_MAX);

        child_object_tree_node->op_object = child_object;
        child_object_tree_node->op_depth = object_tree_node->op_depth + 1;
        TAILQ_INSERT_TAIL(&object_tree_node->op_children, child_object_tree_node,
            op_entries);

        _cached_objects_dirty = true;
        _cached_sorted_objects_dirty = true;
}

/*
 * Remove object and its children.
 */
void
objects_object_remove(const struct object *object)
{
        assert(_initialized);
        assert(object != NULL);

        while (!(TAILQ_EMPTY(&_object_tree))) {
                struct object_tree_node *object_tree_node;
                object_tree_node = TAILQ_FIRST(&_object_tree);

                if (object_tree_node->op_object == object) {
                        /* Clear children */
                        traverse_object_tree_node_remove(object_tree_node, &_object_tree);

                        _cached_objects_dirty = true;
                        _cached_sorted_objects_dirty = true;
                        return;
                }
        }

        assert(false && "Object not found");
}

/*
 * Remove child object and its children.
 */
void
objects_object_child_remove(const struct object *object,
    const struct object *child_object)
{
        assert(_initialized);
        assert(object != NULL);
        assert(child_object != NULL);

        /* Search for object */
        while (!(TAILQ_EMPTY(&_object_tree))) {
                struct object_tree_node *object_tree_node;
                object_tree_node = TAILQ_FIRST(&_object_tree);

                if (object_tree_node->op_object == object) {
                        /* Search for child object */
                        struct object_tree_node *child_object_tree_node;
                        child_object_tree_node = traverse_object_tree_node_find(object_tree_node,
                            child_object);
                        assert(child_object_tree_node != NULL);

                        /* Remove sub-tree */
                        traverse_object_tree_node_remove(child_object_tree_node,
                            &object_tree_node->op_children);

                        _cached_objects_dirty = true;
                        _cached_sorted_objects_dirty = true;
                        return;
                }
        }

        assert(false && "Object not found");
}

const struct objects *
objects_list(void)
{
        assert(_initialized);

        if (_cached_objects_dirty) {
                uint32_t object_idx;
                object_idx = 0;

                _cached_objects[0].parent = NULL;
                _cached_objects[0].object = NULL;

                struct object_tree_node *iter_object_tree_node;
                TAILQ_FOREACH (iter_object_tree_node, &_object_tree, op_entries) {
                        /* Make sure we don't buffer overflow */
                        assert((object_idx + 1) < OBJECTS_MAX);

                        object_idx += traverse_object_tree_node_populate(
                                iter_object_tree_node, &_cached_objects[object_idx]);
                        _cached_objects[object_idx + 1].parent = NULL;
                        _cached_objects[object_idx + 1].object = NULL;
                }
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

        while (!(TAILQ_EMPTY(&_object_tree))) {
                struct object_tree_node *object_tree_node;
                object_tree_node = TAILQ_FIRST(&_object_tree);

                traverse_object_tree_node_remove(object_tree_node, &_object_tree);
        }
}

const struct camera *
objects_component_camera_find(void)
{
        return _cached_camera;
}

/*
 * Find object in tree.
 *
 * If successful, the object is returned. Otherwise, NULL is returned
 * for the following cases:
 *
 *   - Object is not found
 */
static struct object_tree_node *
traverse_object_tree_node_find(struct object_tree_node *object_tree_node,
    const struct object *object)
{
        static uint32_t depth = 1;

        assert(object_tree_node != NULL);
        assert(object != NULL);
        assert(depth <= OBJECTS_DEPTH_MAX);

        /* Visit */
        if (object_tree_node->op_object == object) {
                return object_tree_node;
        }

        if ((TAILQ_EMPTY(&object_tree_node->op_children))) {
                return NULL;
        }

        depth++;

        struct object_tree_node *child_object_tree_node;
        child_object_tree_node = NULL;

        struct object_tree_node *iter_object_tree_node;
        TAILQ_FOREACH (iter_object_tree_node, &object_tree_node->op_children,
            op_entries) {
                if ((child_object_tree_node = traverse_object_tree_node_find(
                                iter_object_tree_node, object)) != NULL) {
                        break;
                }
        }

        depth--;

        return child_object_tree_node;
}

/*
 * Remove object and its children from the tree.
 */
static void
traverse_object_tree_node_remove(struct object_tree_node *object_tree_node,
    struct object_tree *object_tree)
{
        static uint32_t depth = 1;

        assert(object_tree != NULL);
        assert(object_tree_node != NULL);
        assert(depth <= OBJECTS_DEPTH_MAX);

        /* Visit */
        TAILQ_REMOVE(object_tree, object_tree_node, op_entries);

        int error_code;
        error_code = memb_free(&_object_tree_node_pool, object_tree_node);
        assert(error_code == 0);

        if ((TAILQ_EMPTY(&object_tree_node->op_children))) {
                return;
        }

        depth++;

        struct object_tree_node *iter_object_tree_node;
        TAILQ_FOREACH (iter_object_tree_node, &object_tree_node->op_children,
            op_entries) {
                traverse_object_tree_node_remove(iter_object_tree_node,
                    &object_tree_node->op_children);
        }

        depth--;
}

/*
 * Traverse the tree and populate an array of pointers to objects.
 *
 * The number of objects inserted in the array of pointers to objects is
 * returned.
 */
static uint32_t
traverse_object_tree_node_populate(struct object_tree_node *object_tree_node,
    struct objects *objects)
{
        static uint32_t depth = 1;
        static const struct object *parent = NULL;

        assert(object_tree_node != NULL);
        assert(objects != NULL);
        assert(depth <= OBJECTS_DEPTH_MAX);

        if (depth == 1) {
                parent = NULL;
        }

        /* Visit */
        objects[0].parent = parent;
        objects[0].object = object_tree_node->op_object;

        if ((TAILQ_EMPTY(&object_tree_node->op_children))) {
                return 1;
        }

        parent = object_tree_node->op_object;

        depth++;

        uint32_t object_idx;
        object_idx = 0;

        struct object_tree_node *iter_object_tree_node;
        TAILQ_FOREACH (iter_object_tree_node, &object_tree_node->op_children,
            op_entries) {
                object_idx += traverse_object_tree_node_populate(
                        iter_object_tree_node, &objects[object_idx + 1]);
        }

        depth--;

        return 1 + object_idx;
}
