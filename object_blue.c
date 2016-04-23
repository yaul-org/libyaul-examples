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

static void on_init(void);
static void on_update(void);
static void on_draw(void);
static void on_collision(struct object *, const struct collider_info *);
static void on_trigger(struct object *);

static fix16_vector3_t _vertex_list[4] = {
        FIX16_VECTOR3_INITIALIZER( 0.0f,  0.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER( 0.0f, 64.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER(64.0f, 64.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER(64.0f,  0.0f, 1.0f)
};

static uint16_t _color_list[1] = {
        COLOR_RGB888_TO_RGB555(127, 127, 127)
};

static struct collider _collider;
static struct rigid_body _rigid_body;

struct object_blue object_blue = {
        .active = true,
        .id = OBJECT_ID_BLUE,
        .visible = true,
        .vertex_list = &_vertex_list[0],
        .vertex_count = 4,
        .transform = {
                .object = (struct object *)&object_blue,
                .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 1.0f)
        },
        .color_list = &_color_list[0],
        .camera = NULL,
        .rigid_body = &_rigid_body,
        .colliders = &_collider,
        .on_init = on_init,
        .on_update = on_update,
        .on_draw = on_draw,
        .on_destroy = NULL,
        .on_collision = on_collision,
        .on_trigger = on_trigger,
        .functions = {
        },
};

static uint32_t _state;
static uint32_t _last_state;

static void
on_init(void)
{
        _state = BLUE_STATE_WAITING;
        _last_state = _state;
}

static void
on_update(void)
{
        cons_buffer("Hello from blue\n");

        OBJECT(&object_blue, transform).position.x =
            fix16_add(OBJECT(&object_blue, transform).position.x, F16(1.25f));

        OBJECT(&object_blue, transform).position.y =
            fix16_add(OBJECT(&object_blue, transform).position.y, F16(1.0f));

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
on_draw(void)
{
}

static void
on_collision(struct object *other __unused,
    const struct collider_info *info __unused)
{
}

static void
on_trigger(struct object *other __unused)
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
