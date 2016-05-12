/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_OBJECTS_H
#define ENGINE_OBJECTS_H

#include "engine.h"

#define OBJECTS_MAX     256

#define OBJECTS_Z_MIN           0
#define OBJECTS_Z_MAX           7
#define OBJECTS_Z_MAX_BUCKETS   (OBJECTS_Z_MAX + 1)

struct object_bucket_entry;

struct objects {
        /* Sorted objects into separate buckets, indexed by integer Z
         * position */
        STAILQ_HEAD(object_bucket, object_bucket_entry) buckets[OBJECTS_Z_MAX_BUCKETS];

        /* Unsorted list of objects in objects tree */
        const struct object *list[OBJECTS_MAX];
};

struct object_bucket_entry {
        const fix16_vector3_t *position;
        const struct object *object;

        STAILQ_ENTRY(object_bucket_entry) entries;
};

extern void objects_init(void);
extern void objects_clear(void);
extern const struct objects *objects_fetch(void);
extern void objects_object_register(struct object *);
extern void objects_object_unregister(struct object *);
extern bool objects_object_added(const struct object *);
extern void objects_object_add(struct object *);
extern void objects_object_child_add(struct object *, struct object *);
extern void objects_object_remove(struct object *);
extern void objects_object_clear(struct object *);
extern const struct component *objects_component_find(int32_t);
extern void object_component_find_all(const struct object *, int32_t,
    struct component **);

#endif /* !ENGINE_OBJECTS_H */
