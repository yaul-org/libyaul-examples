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

#define OBJECTS_MAX 512

extern void objects_init(void);
extern void objects_object_add(struct object *);
extern void objects_object_remove(struct object *);
extern const struct object **objects_list(void);
extern void objects_clear(void);

#endif /* !ENGINE_OBJECTS_H */
