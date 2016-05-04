/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

#include "object_camera.h"

#define CAMERA_STATE_WAITING              0

static const char *_camera_state2str[] __unused = {
        "CAMERA_STATE_WAITING"
};

static void on_init(struct object *);
static void on_update(struct object *);

static struct camera _camera = {
        .active = true,
        .object = (const struct object *)&object_camera,
        .width = 320,
        .height = 224,
        .on_init = component_camera_init
};

struct object_camera object_camera = {
        .active = true,
        .id = OBJECT_ID_CAMERA,
        .visible = true,
        .vertex_list = NULL,
        .vertex_count = 0,
        .color_list = NULL,
        .transform = {
                .object = (struct object *)&object_camera,
                .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 0.0f)
        },
        .camera = &_camera,
        .rigid_body = NULL,
        .colliders = NULL,
        .on_init = on_init,
        .on_update = on_update,
        .on_draw = NULL,
        .on_destroy = NULL,
        .on_collision = NULL,
        .on_trigger = NULL,
        .functions = {
        },
};

static uint32_t _state;
static uint32_t _last_state;

static void
on_init(struct object *this __unused)
{
        _state = CAMERA_STATE_WAITING;
        _last_state = _state;
}

static void
on_update(struct object *this __unused)
{
        cons_buffer("Hello from camera\n");

        /* OBJECT(&object_camera, transform).position.x = */
        /*     fix16_add(OBJECT(&object_camera, transform).position.x, F16(1.0f)); */

        switch (_state) {
        case CAMERA_STATE_WAITING:
                break;
        default:
                assert(false && "Invalid state");
        }
}
