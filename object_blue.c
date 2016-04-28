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

#define COMPONENT_JETPACK_STATE_PARTICLE_WAITING        0
#define COMPONENT_JETPACK_STATE_PARTICLE_INIT           1
#define COMPONENT_JETPACK_STATE_PARTICLE_BLAST_OFF      2
#define COMPONENT_JETPACK_STATE_PARTICLE_DESTROY        3

static void component_jetpack_on_init(void);
static void component_jetpack_on_update(void);
static bool component_jetpack_fncn_blasting(void);
static void component_jetpack_fncn_blast_off(void);

static fix16_vector3_t _vertex_list[4] = {
        FIX16_VECTOR3_INITIALIZER( 0.0f,  0.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER( 0.0f, 64.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER(64.0f, 64.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER(64.0f,  0.0f, 1.0f)
};

static color_rgb_t _color_list[1] = {
        {{15, 15, 15}}
};

static struct collider _collider = {
        .active = true,
        .object = (const struct object *)&object_blue
};

static struct rigid_body _rigid_body = {
        .active = true,
        .object = (const struct object *)&object_blue
};

static struct component_jetpack {
        COMPONENT_DECLARATIONS

        struct {
                uint32_t m_state;
                uint32_t m_last_state;

                struct object_particle *m_object_particle_list[32];
                uint32_t m_object_particle_count;
        } data;

        struct {
                bool (*m_blasting)(void);
                void (*m_blast_off)(void);
                void (*m_explode)(void);
        } functions;
} _component_jetpack = {
        .active = true,
        .object = (const struct object *)&object_blue,
        .on_init = component_jetpack_on_init,
        .on_update = component_jetpack_on_update,
        .data = {
                .m_state = COMPONENT_JETPACK_STATE_PARTICLE_WAITING,
                .m_last_state = COMPONENT_JETPACK_STATE_PARTICLE_WAITING,
                .m_object_particle_list = {
                        NULL
                },
                .m_object_particle_count = 0,
        },
        .functions = {
                .m_blasting = component_jetpack_fncn_blasting,
                .m_blast_off = component_jetpack_fncn_blast_off,
                .m_explode = NULL
        }
};

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
        .component_list = {
                (struct component *)&_component_jetpack
        },
        .component_count = 1,
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
on_init(struct object *this __unused)
{
        _state = BLUE_STATE_WAITING;
        _last_state = _state;

        object_component_init((const struct object *)&object_blue);

        THIS(object_blue, initialized) = true;
}

static void
on_update(struct object *this)
{
        assert(THIS(object_blue, initialized));

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
on_draw(struct object *this __unused)
{
        assert(THIS(object_blue, initialized));
}

static void
on_collision(struct object *this,
    const struct object *other __unused,
    const struct collider_info *info __unused)
{
        assert(THIS(object_blue, initialized));
}

static void
on_trigger(struct object *this, const struct object *other __unused)
{
        assert(THIS(object_blue, initialized));
}

static void
component_jetpack_on_init(void)
{
        struct object_particle **object_particle_list;
        object_particle_list = COMPONENT_PUBLIC_DATA(&_component_jetpack,
            object_particle_list);

        uint32_t object_particle_count;
        object_particle_count = sizeof(COMPONENT_PUBLIC_DATA(
                    &_component_jetpack, object_particle_list)) / sizeof(struct object *);

        COMPONENT_PUBLIC_DATA(&_component_jetpack, object_particle_count) =
            object_particle_count;

        uint32_t object_idx;
        for (object_idx = 0; object_idx < object_particle_count; object_idx++) {
                struct object_particle *object_particle;
                object_particle = particle_alloc();

                OBJECT(object_particle, active) = false;
                OBJECT(object_particle, id) =
                    OBJECT_ID_PARTICLE_BEGIN + object_idx;

                OBJECT_PUBLIC_DATA(object_particle, ttl) = 5;
                OBJECT_PUBLIC_DATA(object_particle, color_from).r = 31;
                OBJECT_PUBLIC_DATA(object_particle, color_from).g = 31;
                OBJECT_PUBLIC_DATA(object_particle, color_from).b = 31;
                OBJECT_PUBLIC_DATA(object_particle, color_to).r = 31;
                OBJECT_PUBLIC_DATA(object_particle, color_to).g = 31;
                OBJECT_PUBLIC_DATA(object_particle, color_to).b = 31;
                OBJECT_PUBLIC_DATA(object_particle, delta).x = F16(0.0f);
                OBJECT_PUBLIC_DATA(object_particle, delta).y = F16(0.0f);

                OBJECT_INIT(object_particle);

                object_particle_list[object_idx] = object_particle;
        }

        COMPONENT_PUBLIC_DATA(&_component_jetpack, state) =
            COMPONENT_JETPACK_STATE_PARTICLE_WAITING;
        COMPONENT_PUBLIC_DATA(&_component_jetpack, last_state) =
            COMPONENT_JETPACK_STATE_PARTICLE_WAITING;
}

static void
component_jetpack_on_update(void)
{
        cons_buffer("Hello from component jetpack\n");

        uint32_t object_idx;
        object_idx = 0;

        const struct object *object;
        object = COMPONENT(&_component_jetpack, object);

        uint32_t object_particle_count;
        object_particle_count = COMPONENT_PUBLIC_DATA(&_component_jetpack,
            object_particle_count);
        struct object_particle **object_particle_list;
        object_particle_list = COMPONENT_PUBLIC_DATA(&_component_jetpack,
            object_particle_list);

        uint32_t *state;
        state = &COMPONENT_PUBLIC_DATA(&_component_jetpack, state);

        switch (*state) {
        case COMPONENT_JETPACK_STATE_PARTICLE_WAITING:
                *state = COMPONENT_JETPACK_STATE_PARTICLE_INIT;
                break;
        case COMPONENT_JETPACK_STATE_PARTICLE_INIT:
                for (object_idx = 0; object_idx < object_particle_count;
                     object_idx++) {
                        struct object_particle *object_particle;
                        object_particle = object_particle_list[object_idx];

                        OBJECT(object_particle, active) = true;

                        objects_object_child_add((struct object *)object,
                            (struct object *)object_particle);
                }

                *state = COMPONENT_JETPACK_STATE_PARTICLE_BLAST_OFF;
                break;
        case COMPONENT_JETPACK_STATE_PARTICLE_BLAST_OFF:
                for (object_idx = 0; object_idx < object_particle_count;
                     object_idx++) {
                        struct object_particle *object_particle __unused;
                        object_particle = object_particle_list[object_idx];
                }
                break;
        case COMPONENT_JETPACK_STATE_PARTICLE_DESTROY:
                break;
        }
}

static bool
component_jetpack_fncn_blasting(void)
{
        return false;
}

static void
component_jetpack_fncn_blast_off(void)
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
