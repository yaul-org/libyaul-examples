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
#define OBJECTS_Z_MAX           15
#define OBJECTS_Z_MAX_BUCKETS   (OBJECTS_Z_MAX + 1)

struct object_z_entry;

struct objects {
        STAILQ_HEAD(object_bucket, object_z_entry) buckets[OBJECTS_Z_MAX_BUCKETS];
};

struct object_z_entry {
        const struct object_z *object_z;

        STAILQ_ENTRY(object_z_entry) entries;
};

struct object_z {
        const struct object *object;
        const fix16_vector3_t *position;
};

extern void objects_init(void);
extern void objects_object_register(struct object *);
extern void objects_object_unregister(struct object *);
extern bool objects_object_added(const struct object *);
extern void objects_object_add(struct object *);
extern void objects_object_child_add(struct object *, struct object *);
extern void objects_object_remove(struct object *);
extern void objects_object_clear(struct object *);
extern void objects_clear(void);
extern const struct object_z *objects_list(void);
extern const struct objects *objects_sorted_list(void);
extern const struct component *objects_component_find(int32_t);

#endif /* !ENGINE_OBJECTS_H */
