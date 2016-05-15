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
        .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 0.0f)
};

static struct camera _camera = {
        .active = true,
        .id = COMPONENT_ID_CAMERA,
        .object = (const struct object *)&object_camera,
        .on_init = component_camera_on_init,
        .on_update = component_camera_on_update,
        .width = 320,
        .height = 224,
        .functions = {
                .m_on = component_camera_on,
                .m_off = component_camera_off
        }
};

static struct camera_mgr _camera_mgr = {
        .active = true,
        .id = COMPONENT_ID_CAMERA_MGR,
        .object = (struct object *)&object_camera,
        .on_init = &component_camera_mgr_on_init,
        .on_update = &component_camera_mgr_on_update,
        .on_draw = &component_camera_mgr_on_draw,
        .on_destroy = &component_camera_mgr_on_destroy,
        .start_delay = 0,
        .speed = F16(0.3333333f),
};

struct object object_camera = {
        .active = true,
        .id = OBJECT_ID_CAMERA,
        .component_list = {
                OBJECT_COMPONENT_INITIALIZER(transform, &_transform),
                OBJECT_COMPONENT_INITIALIZER(camera, &_camera),
                OBJECT_COMPONENT_INITIALIZER(camera_mgr, &_camera_mgr)
        },
        .component_count = 3
};
