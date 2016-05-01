/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdbool.h>
#include <stdio.h>

#include "particle.h"

#define PARTICLE_COUNT_MAX 128

#define PARTICLE_STATE_WAITING  0

static const char *_particle_state2str[] __unused = {
        "PARTICLE_STATE_WAITING"
};

MEMB(_object_particle_pool, struct object_particle, PARTICLE_COUNT_MAX,
    sizeof(struct object_particle));
MEMB(_collider_pool, struct collider, PARTICLE_COUNT_MAX,
    sizeof(struct collider));
MEMB(_rigid_body_pool, struct rigid_body, PARTICLE_COUNT_MAX,
    sizeof(struct rigid_body));
MEMB(_color_list_pool, color_rgb_t, PARTICLE_COUNT_MAX,
    sizeof(color_rgb_t));

static void object_particle_on_init(struct object *);
static void object_particle_on_update(struct object *);
static void object_particle_on_draw(struct object *);

static bool _initialized;

static fix16_vector3_t _vertex_list[4] = {
        FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER(0.0f, 4.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER(4.0f, 4.0f, 1.0f),
        FIX16_VECTOR3_INITIALIZER(4.0f, 0.0f, 1.0f)
};

void
particle_init(void)
{
        if (_initialized) {
                return;
        }

        memb_init(&_object_particle_pool);
        memb_init(&_collider_pool);
        memb_init(&_rigid_body_pool);
        memb_init(&_color_list_pool);

        _initialized = true;
}

struct object_particle *
particle_alloc(void)
{
        assert(_initialized);

        struct object_particle *object_particle;
        object_particle =
            (struct object_particle *)memb_alloc(&_object_particle_pool);
        assert(object_particle != NULL);

        struct transform *transform;
        transform = &OBJECT(object_particle, transform);

        COMPONENT(transform, object) = (const struct object *)object_particle;
        COMPONENT(transform, position).x = F16(0.0f);
        COMPONENT(transform, position).y = F16(0.0f);
        COMPONENT(transform, position).z = F16(1.0f);

        struct collider *collider;
        collider =
            (struct collider *)memb_alloc(&_collider_pool);
        assert(collider != NULL);

        COMPONENT(collider, active) = true;
        COMPONENT(collider, object) = (const struct object *)object_particle;

        struct rigid_body *rigid_body;
        rigid_body =
            (struct rigid_body *)memb_alloc(&_rigid_body_pool);
        assert(rigid_body != NULL);

        COMPONENT(rigid_body, active) = true;
        COMPONENT(rigid_body, object) = (const struct object *)object_particle;

        color_rgb_t *color_list;
        color_list = (color_rgb_t *)memb_alloc(&_color_list_pool);
        assert(color_list != NULL);

        color_list[0].r = 31;
        color_list[0].g = 31;
        color_list[0].b = 31;

        OBJECT(object_particle, active) = false;
        OBJECT(object_particle, id) = -1;
        OBJECT(object_particle, visible) = true;
        OBJECT(object_particle, vertex_list) = &_vertex_list[0];
        OBJECT(object_particle, vertex_count) = 4;
        OBJECT(object_particle, color_list) = color_list;

        OBJECT(object_particle, camera) = NULL;
        OBJECT(object_particle, rigid_body) = rigid_body;
        OBJECT(object_particle, colliders) = collider;

        uint32_t component_idx;
        for (component_idx = 0; component_idx < OBJECT_COMPONENT_LIST_MAX;
             component_idx++) {
                OBJECT(object_particle, component_list)[component_idx] = NULL;
        }

        OBJECT(object_particle, component_count) = 0;
        OBJECT(object_particle, initialized) = false;
        OBJECT(object_particle, on_init) = object_particle_on_init;
        OBJECT(object_particle, on_update) = object_particle_on_update;
        OBJECT(object_particle, on_draw) = object_particle_on_draw;
        OBJECT(object_particle, on_destroy) = NULL;
        OBJECT(object_particle, on_collision) = NULL;
        OBJECT(object_particle, on_trigger) = NULL;

        /* Public data */
        OBJECT_PUBLIC_DATA(object_particle, ttl) = 0;

        OBJECT_PUBLIC_DATA(object_particle, color_from).r = 31;
        OBJECT_PUBLIC_DATA(object_particle, color_from).g = 31;
        OBJECT_PUBLIC_DATA(object_particle, color_from).b = 31;

        OBJECT_PUBLIC_DATA(object_particle, color_to).r = 31;
        OBJECT_PUBLIC_DATA(object_particle, color_to).g = 31;
        OBJECT_PUBLIC_DATA(object_particle, color_to).b = 31;
        OBJECT_PUBLIC_DATA(object_particle, color_to).b = 31;

        OBJECT_PUBLIC_DATA(object_particle, delta).x = F16(0.0f);
        OBJECT_PUBLIC_DATA(object_particle, delta).y = F16(0.0f);

        return object_particle;
}

void
particle_free(struct object_particle *object_particle)
{
        assert(_initialized);
        assert(object_particle != NULL);

        struct rigid_body *rigid_body;
        rigid_body = OBJECT(object_particle, rigid_body);
        assert(rigid_body != NULL);
        COMPONENT(rigid_body, active) = false;
        memb_free(&_rigid_body_pool, rigid_body);

        struct collider *collider;
        collider = &OBJECT(object_particle, colliders)[0];
        assert(collider != NULL);
        COMPONENT(collider, active) = false;
        memb_free(&_collider_pool, collider);

        color_rgb_t *color_list;
        color_list = OBJECT(object_particle, color_list);
        assert(color_list != NULL);
        memb_free(&_color_list_pool, color_list);

        assert(object_particle != NULL);
        OBJECT(object_particle, active) = false;
        memb_free(&_object_particle_pool, object_particle);
}

static void
object_particle_on_init(struct object *this __unused)
{
        THIS(object_particle, initialized) = true;

        /* Compute table of color HSV values */
        /* Compute table of HSV to RGB mapping */
}

static void
object_particle_on_update(struct object *this __unused)
{
        assert(THIS(object_particle, initialized));

        if (THIS_PUBLIC_DATA(object_particle, ttl) == 0) {
                return;
        }

        THIS_PUBLIC_DATA(object_particle, ttl)--;

        fix16_vector2_add((fix16_vector2_t *)&THIS(object_particle, transform).position,
            &THIS_PUBLIC_DATA(object_particle, delta),
            (fix16_vector2_t *)&THIS(object_particle, transform).position);
}

static void
object_particle_on_draw(struct object *this __unused)
{
        assert(THIS(object_particle, initialized));
}
