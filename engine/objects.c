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

struct object_ptr;

TAILQ_HEAD(object_ptrs, object_ptr);

struct object_ptr {
        const struct object *op_object;
        struct object_ptrs op_children;
        uint32_t op_depth;

        TAILQ_ENTRY(object_ptr) op_entries;
} __aligned(256);

static bool _initialized = false;
struct object_ptrs _object_ptrs;

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

MEMB(_object_ptr_pool, struct object_ptr, OBJECTS_MAX,
    sizeof(struct object_ptr));

static struct object_ptr *traverse_object_ptr_find(struct object_ptr *,
    const struct object *);
static uint32_t traverse_object_ptr_populate(struct object_ptr *,
    struct objects *);
static void traverse_object_ptr_remove(struct object_ptr *,
    struct object_ptrs *);

/*
 * Initialize objects system.
 */
void
objects_init(void)
{
        if (_initialized) {
                return;
        }

        TAILQ_INIT(&_object_ptrs);

        memb_init(&_object_ptr_pool);

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

        struct object_ptr *iter_object_ptr;
        iter_object_ptr = NULL;
        TAILQ_FOREACH (iter_object_ptr, &_object_ptrs, op_entries) {
                const struct object *cur_object;
                cur_object = iter_object_ptr->op_object;

                /* No duplicate objects can be added */
                assert(cur_object->id != object->id);
        }

        /* Caching */
        if (object->camera != NULL) {
                _cached_camera = object->camera;
        }

        struct object_ptr *object_ptr;
        object_ptr = (struct object_ptr *)memb_alloc(&_object_ptr_pool);
        assert(object_ptr != NULL);

        object_ptr->op_object = object;
        TAILQ_INIT(&object_ptr->op_children);
        object_ptr->op_depth = 1;

        TAILQ_INSERT_TAIL(&_object_ptrs, object_ptr, op_entries);

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

        struct object_ptr *object_ptr;
        object_ptr = NULL;

        struct object_ptr *iter_object_ptr;

        /* Look for the corresponding object */
        TAILQ_FOREACH (iter_object_ptr, &_object_ptrs, op_entries) {
                const struct object *cur_object;
                cur_object = iter_object_ptr->op_object;

                if (cur_object == object) {
                        object_ptr = iter_object_ptr;
                        break;
                }
        }
        assert(object_ptr != NULL);

        /* Check for duplicates in object's children */
        TAILQ_FOREACH (iter_object_ptr, &object_ptr->op_children, op_entries) {
                const struct object *cur_object;
                cur_object = iter_object_ptr->op_object;

                /* No duplicate objects can be added */
                assert(cur_object->id != object->id);
        }

        /* Allocate child object */
        struct object_ptr *child_object_ptr;
        child_object_ptr = (struct object_ptr *)memb_alloc(&_object_ptr_pool);
        assert(child_object_ptr != NULL);

        /* Assert we don't have a "tall" tree */
        assert((object_ptr->op_depth + 1) <= OBJECTS_DEPTH_MAX);

        child_object_ptr->op_object = child_object;
        child_object_ptr->op_depth = object_ptr->op_depth + 1;
        TAILQ_INSERT_TAIL(&object_ptr->op_children, child_object_ptr,
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

        while (!(TAILQ_EMPTY(&_object_ptrs))) {
                struct object_ptr *object_ptr;
                object_ptr = TAILQ_FIRST(&_object_ptrs);

                if (object_ptr->op_object == object) {
                        /* Clear children */
                        traverse_object_ptr_remove(object_ptr, &_object_ptrs);

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
        while (!(TAILQ_EMPTY(&_object_ptrs))) {
                struct object_ptr *object_ptr;
                object_ptr = TAILQ_FIRST(&_object_ptrs);

                if (object_ptr->op_object == object) {
                        /* Search for child object */
                        struct object_ptr *child_object_ptr;
                        child_object_ptr = traverse_object_ptr_find(object_ptr,
                            child_object);
                        assert(child_object_ptr != NULL);

                        /* Remove sub-tree */
                        traverse_object_ptr_remove(child_object_ptr,
                            &object_ptr->op_children);

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

                struct object_ptr *iter_object_ptr;
                TAILQ_FOREACH (iter_object_ptr, &_object_ptrs, op_entries) {
                        /* Make sure we don't buffer overflow */
                        assert((object_idx + 1) < OBJECTS_MAX);

                        object_idx += traverse_object_ptr_populate(
                                iter_object_ptr, &_cached_objects[object_idx]);
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

        while (!(TAILQ_EMPTY(&_object_ptrs))) {
                struct object_ptr *object_ptr;
                object_ptr = TAILQ_FIRST(&_object_ptrs);

                traverse_object_ptr_remove(object_ptr, &_object_ptrs);
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
static struct object_ptr *
traverse_object_ptr_find(struct object_ptr *object_ptr,
    const struct object *object)
{
        static uint32_t depth = 1;

        assert(object_ptr != NULL);
        assert(object != NULL);
        assert(depth <= OBJECTS_DEPTH_MAX);

        /* Visit */
        if (object_ptr->op_object == object) {
                return object_ptr;
        }

        if ((TAILQ_EMPTY(&object_ptr->op_children))) {
                return NULL;
        }

        depth++;

        struct object_ptr *child_object_ptr;
        child_object_ptr = NULL;

        struct object_ptr *iter_object_ptr;
        TAILQ_FOREACH (iter_object_ptr, &object_ptr->op_children, op_entries) {
                if ((child_object_ptr = traverse_object_ptr_find(
                                iter_object_ptr, object)) != NULL) {
                        break;
                }
        }

        depth--;

        return child_object_ptr;
}

/*
 * Remove object and its children from the tree.
 */
static void
traverse_object_ptr_remove(struct object_ptr *object_ptr,
    struct object_ptrs *object_ptrs)
{
        static uint32_t depth = 1;

        assert(object_ptrs != NULL);
        assert(object_ptr != NULL);
        assert(depth <= OBJECTS_DEPTH_MAX);

        /* Visit */
        TAILQ_REMOVE(object_ptrs, object_ptr, op_entries);

        int error_code;
        error_code = memb_free(&_object_ptr_pool, object_ptr);
        assert(error_code == 0);

        if ((TAILQ_EMPTY(&object_ptr->op_children))) {
                return;
        }

        depth++;

        struct object_ptr *iter_object_ptr;
        TAILQ_FOREACH (iter_object_ptr, &object_ptr->op_children, op_entries) {
                traverse_object_ptr_remove(iter_object_ptr,
                    &object_ptr->op_children);
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
traverse_object_ptr_populate(struct object_ptr *object_ptr,
    struct objects *objects)
{
        static uint32_t depth = 1;

        assert(object_ptr != NULL);
        assert(objects != NULL);
        assert(depth <= OBJECTS_DEPTH_MAX);

        /* Visit */
        objects[0].parent = NULL;
        objects[0].object = object_ptr->op_object;

        if ((TAILQ_EMPTY(&object_ptr->op_children))) {
                return 1;
        }

        depth++;

        uint32_t object_idx;
        object_idx = 0;

        struct object_ptr *iter_object_ptr;
        TAILQ_FOREACH (iter_object_ptr, &object_ptr->op_children, op_entries) {
                object_idx += traverse_object_ptr_populate(iter_object_ptr,
                    &objects[object_idx + 1]);
        }

        depth--;

        return 1 + object_idx;
}
