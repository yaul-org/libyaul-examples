/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

#include "object_world.h"

#define WORLD_STATE_WAITING              0

static const char *_world_state2str[] __unused = {
        "WORLD_STATE_WAITING"
};

static void on_init(void);
static void on_update(void);
static void on_draw(void);

struct object_world object_world = {
        .active = true,
        .id = OBJECT_ID_WORLD,
        .visible = true,
        .vertex_list = NULL,
        .vertex_count = 0,
        .color_list = NULL,
        .transform = {
                .object = (struct object *)&object_world,
                .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 2.0f)
        },
        .camera = NULL,
        .rigid_body = NULL,
        .colliders = NULL,
        .on_init = on_init,
        .on_update = on_update,
        .on_draw = on_draw,
        .on_destroy = NULL,
        .on_collision = NULL,
        .on_trigger = NULL,
        .functions = {
        },
};

static uint32_t _state;
static uint32_t _last_state;

static void
on_init(void)
{
        _state = WORLD_STATE_WAITING;
        _last_state = _state;

        object_component_init((const struct object *)&object_world);
}

static void
on_update(void)
{
        cons_buffer("Hello from world\n");

        switch (_state) {
        case WORLD_STATE_WAITING:
                break;
        default:
                assert(false && "Invalid state");
        }
}

static void
on_draw(void)
{
}
