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

struct object_ptr {
        struct object *op_object;

        TAILQ_ENTRY(object_ptr) op_entries;
} __aligned(8);

TAILQ_HEAD(object_ptrs, object_ptr);

static bool _initialized = false;
struct object_ptrs _object_ptrs_pool;

/* Caching */
/* Cached list of objects allocated */
static bool _cached_objects_dirty = true;
static struct object *_cached_objects[OBJECTS_MAX];
static bool _cached_sorted_objects_dirty = true;
static struct object *_cached_sorted_objects[OBJECTS_MAX];
/* Cached list of objects last searched using objects_component_find() */
static struct object *_cached_objects_component[OBJECTS_MAX];
/* Cached pointer to camera component */
static const struct camera *_cached_camera = NULL;

MEMB(object_ptr_pool, struct object_ptr, OBJECTS_MAX,
    sizeof(struct object_ptr));

void
objects_init(void)
{
        if (_initialized) {
                return;
        }

        TAILQ_INIT(&_object_ptrs_pool);

        memb_init(&object_ptr_pool);

        uint32_t object_idx;
        for (object_idx = 0; object_idx < OBJECTS_MAX; object_idx++) {
                _cached_objects[object_idx] = NULL;
        }

        _initialized = true;
}

void
objects_object_add(struct object *object)
{
        assert(_initialized);
        assert(object != NULL);

        struct object_ptr *object_ptr;
        object_ptr = NULL;
        TAILQ_FOREACH (object_ptr, &_object_ptrs_pool, op_entries) {
                struct object *cur_object;
                cur_object = object_ptr->op_object;

                /* No duplicate objects can be added */
                assert(cur_object->id != object->id);
        }

        /* Caching */
        if (object->camera != NULL) {
                _cached_camera = object->camera;
        }

        object_ptr = (struct object_ptr *)memb_alloc(&object_ptr_pool);
        assert(object_ptr != NULL);

        object_ptr->op_object = object;

        TAILQ_INSERT_TAIL(&_object_ptrs_pool, object_ptr, op_entries);

        _cached_objects_dirty = true;
}

void
objects_object_remove(struct object *object)
{
        assert(_initialized);
        assert(object != NULL);

        while (!(TAILQ_EMPTY(&_object_ptrs_pool))) {
                struct object_ptr *object_ptr;
                object_ptr = TAILQ_FIRST(&_object_ptrs_pool);

                if (object_ptr->op_object == object) {
                        int error_code;
                        error_code = memb_free(&object_ptr_pool, object_ptr);

                        assert(error_code == 0);

                        TAILQ_REMOVE(&_object_ptrs_pool, object_ptr, op_entries);

                        _cached_objects_dirty = true;
                        return;
                }
        }

        assert(false && "Object not found");
}

const struct object **
objects_list(void)
{
        assert(_initialized);

        if (_cached_objects_dirty) {
                uint32_t object_idx;
                object_idx = 0;

                struct object_ptr *object_ptr;
                object_ptr = NULL;
                TAILQ_FOREACH (object_ptr, &_object_ptrs_pool, op_entries) {
                        struct object *object;
                        object = object_ptr->op_object;

                        _cached_objects[object_idx] = object;
                        object_idx++;
                }

                _cached_objects[object_idx] = NULL;
        }

        _cached_objects_dirty = false;

        return (const struct object **)&_cached_objects[0];
}

const struct object **
objects_sorted_list(void)
{
        assert(_initialized);

        if (!_cached_sorted_objects_dirty) {
                goto return_list;
        }

        uint32_t object_idx;
        object_idx = 0;

        int32_t z_value;
        for (z_value = OBJECTS_Z_MAX; z_value > 0; z_value--) {
                struct object_ptr *object_ptr;
                object_ptr = NULL;

                TAILQ_FOREACH (object_ptr, &_object_ptrs_pool, op_entries) {
                        struct object *object;
                        object = object_ptr->op_object;

                        int32_t transform_z;
                        transform_z = fix16_to_int(OBJECT_COMPONENT(
                                    object, transform).position.z);

                        if (transform_z == z_value) {
                                _cached_sorted_objects[object_idx] = object;
                                object_idx++;
                        }
                }
        }

        _cached_sorted_objects[object_idx] = NULL;

        _cached_sorted_objects_dirty = false;

return_list:
        return (const struct object **)&_cached_sorted_objects[0];
}

void
objects_clear(void)
{
        assert(_initialized);

        /* Clear cached */
        _cached_camera = NULL;
        _cached_objects[0] = NULL;
        _cached_objects_component[0] = NULL;

        while (!(TAILQ_EMPTY(&_object_ptrs_pool))) {
                struct object_ptr *object_ptr;
                object_ptr = TAILQ_FIRST(&_object_ptrs_pool);

                int error_code;
                error_code = memb_free(&object_ptr_pool, object_ptr);
                assert(error_code == 0);

                TAILQ_REMOVE(&_object_ptrs_pool, object_ptr, op_entries);
        }
}

const struct camera *
objects_component_camera_find(void)
{
        return _cached_camera;
}

#if 0
const struct object **
objects_component_find(uint32_t component_offset)
{
        assert(_initialized);
        assert(object != NULL);

        uint32_t component_idx;
        component_idx = 0;

        _cached_objects_component[0] = NULL;

        struct object_ptr *object_ptr;
        object_ptr = NULL;
        TAILQ_FOREACH (object_ptr, &_object_ptrs_pool, op_entries) {
                struct object *object;
                object = object_ptr->op_object;

                if (object->component_count == 0) {
                        continue;
                }

                if (object->component_list[offset].active) {
                        _cached_objects_component[component_idx] =
                            (const struct object *)object;
                }
        }

        return (const struct object **)&_cached_objects_component[0];
}
#endif
