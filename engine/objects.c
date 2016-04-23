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
static struct object *_objects[OBJECTS_MAX];

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
                _objects[object_idx] = NULL;
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

        object_ptr = (struct object_ptr *)memb_alloc(&object_ptr_pool);
        assert(object_ptr != NULL);

        object_ptr->op_object = object;

        TAILQ_INSERT_TAIL(&_object_ptrs_pool, object_ptr, op_entries);
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
                        return;
                }
        }

        assert(false && "Object not found");
}

const struct object **
objects_list(void)
{
        assert(_initialized);

        if (TAILQ_EMPTY(&_object_ptrs_pool)) {
                _objects[0] = NULL;
        }

        uint32_t object_idx;
        object_idx = 0;

        struct object_ptr *object_ptr;
        object_ptr = NULL;
        TAILQ_FOREACH (object_ptr, &_object_ptrs_pool, op_entries) {
                struct object *object;
                object = object_ptr->op_object;

                _objects[object_idx] = object;
                object_idx++;
        }

        _objects[object_idx] = NULL;

        return (const struct object **)&_objects[0];
}

void
objects_clear(void)
{
        assert(_initialized);

        while (!(TAILQ_EMPTY(&_object_ptrs_pool))) {
                struct object_ptr *object_ptr;
                object_ptr = TAILQ_FIRST(&_object_ptrs_pool);

                int error_code;
                error_code = memb_free(&object_ptr_pool, object_ptr);
                assert(error_code == 0);

                TAILQ_REMOVE(&_object_ptrs_pool, object_ptr, op_entries);
        }
}
