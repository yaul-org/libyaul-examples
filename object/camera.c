/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

static struct transform _transform = {
        .active = true,
        .id = COMPONENT_ID_TRANSFORM,
        .object = (struct object *)&object_camera,
        .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 2.0f)
};

static struct camera _camera = {
        .active = true,
        .id = COMPONENT_ID_CAMERA,
        .object = (const struct object *)&object_camera,
        .width = 320,
        .height = 224,
        .on_init = component_camera_on_init,
        .on_update = component_camera_on_update
};

struct object object_camera = {
        .active = true,
        .id = OBJECT_ID_CAMERA,
        .component_list = {
                OBJECT_COMPONENT_INITIALIZER(transform, &_transform),
                OBJECT_COMPONENT_INITIALIZER(camera, &_camera)
        },
        .component_count = 2
};
