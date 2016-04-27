/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_OBJECTS_H
#define ENGINE_OBJECTS_H

#include <math.h>
#include <inttypes.h>

#include "object.h"

#define OBJECTS_MAX     256

#define OBJECTS_Z_MIN   0
#define OBJECTS_Z_MAX   15

struct objects {
        const struct object *parent;
        const struct object *object;
};

extern void objects_init(void);
extern void objects_object_add(const struct object *);
extern void objects_object_remove(const struct object *);
extern void objects_object_child_add(const struct object *,
    const struct object *);
extern void objects_object_child_remove(const struct object *,
    const struct object *);
extern const struct camera *objects_component_camera_find(void);
extern const struct objects *objects_list(void);
extern const struct objects *objects_sorted_list(void);
extern void objects_clear(void);

#endif /* !ENGINE_OBJECTS_H */
