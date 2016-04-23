/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_PHYSICS_H
#define ENGINE_PHYSICS_H

#include <math.h>

#include "aabb.h"
#include "collider.h"
#include "object.h"
#include "rigid_body.h"

void physics_init(void);
void physics_object_add(struct object *);
void physics_update(void);

#endif /* !ENGINE_PHYSICS_H */
