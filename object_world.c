/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

#include "object_world.h"

static struct transform _transform = {
        .active = true,
        .id = COMPONENT_ID_TRANSFORM,
        .object = (struct object *)&object_world,
        .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 15.0f)
};

struct object_world object_world = {
        .active = true,
        .id = OBJECT_ID_WORLD,
        .component_list = {
                (struct component *)&_transform,
        },
        .component_count = 1
};

