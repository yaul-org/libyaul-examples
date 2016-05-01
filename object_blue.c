/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

#include "object_blue.h"

#define BLUE_STATE_WAITING              0
#define BLUE_STATE_IDLE                 1
#define BLUE_STATE_JET_PACK             2
#define BLUE_STATE_DEAD                 3

static const char *_blue_state2str[] __unused = {
        "BLUE_STATE_WAITING",
        "BLUE_STATE_IDLE",
        "BLUE_STATE_JET_PACK",
        "BLUE_STATE_DEAD"
};

static void on_init(struct object *);
static void on_update(struct object *);
static void on_draw(struct object *);
static void on_collision(struct object *, const struct object *,
    const struct collider_info *);
static void on_trigger(struct object *, const struct object *);

static fix16_vector3_t _vertex_list[4] = {
        FIX16_VECTOR3_INITIALIZER( 0.0f,  0.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER( 0.0f, 64.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER(64.0f, 64.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER(64.0f,  0.0f, 1.0f)
};

static color_rgb555_t _color_list[1] = {
        {{15, 0, 0}}
};

static struct collider _collider = {
        .active = true,
        .object = (const struct object *)&object_blue
};

static struct rigid_body _rigid_body = {
        .active = true,
        .object = (const struct object *)&object_blue
};

static struct particle _component_particle = {
        .active = true,
        .object = (const struct object *)&object_blue,
        .on_init = component_particle_init,
        .on_update = component_particle_update,
        .on_draw = NULL,
        .on_destroy = NULL,
        .looping = true,
        .max_count = 32,
        .emmission_count = 32,
        .ttl = PARTICLE_TTL_MAX,
        .color_from = {
                COLOR_RGB888_INITIALIZER(255,   0,   0)
        },
        .color_to = {
                COLOR_RGB888_INITIALIZER(  0, 255,   0)
        }
};

struct object_blue object_blue = {
        .active = true,
        .id = OBJECT_ID_BLUE,
        .visible = true,
        .vertex_list = &_vertex_list[0],
        .vertex_count = 4,
        .transform = {
                .active = true,
                .object = (struct object *)&object_blue,
                .position = FIX16_VECTOR3_INITIALIZER(128.0f, 80.0f, 2.0f)
        },
        .color_list = &_color_list[0],
        .camera = NULL,
        .rigid_body = &_rigid_body,
        .colliders = &_collider,
        .component_list = {
                (struct component *)&_component_particle
        },
        .component_count = 1,
        .on_init = on_init,
        .on_update = on_update,
        .on_draw = on_draw,
        .on_destroy = NULL,
        .on_collision = on_collision,
        .on_trigger = on_trigger,
        .functions = {
        }
};

static uint32_t _state;
static uint32_t _last_state;

static void
on_init(struct object *this __unused)
{
        _state = BLUE_STATE_WAITING;
        _last_state = _state;

        object_component_init((const struct object *)&object_blue);
}

static void
on_update(struct object *this __unused)
{
        cons_buffer("Hello from blue\n");

        switch (_state) {
        case BLUE_STATE_WAITING:
                break;
        case BLUE_STATE_IDLE:
                break;
        case BLUE_STATE_JET_PACK:
                break;
        case BLUE_STATE_DEAD:
                break;
        default:
                assert(false && "Invalid state");
        }
}

static void
on_draw(struct object *this __unused)
{
}

static void
on_collision(struct object *this __unused, const struct object *other __unused,
    const struct collider_info *info __unused)
{
}

static void
on_trigger(struct object *this __unused, const struct object *other __unused)
{
}

static void __unused
m_update_input(void)
{

        if (digital_pad.connected == 1) {
                if (digital_pad.pressed.button.a ||
                    digital_pad.pressed.button.c) {
                }
        }
}
