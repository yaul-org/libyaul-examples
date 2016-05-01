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
#include "objects.h"

static void particles_init(void);
static struct object_particle *particles_particle_alloc(void);
static void particles_particle_free(struct object_particle *) __unused;

MEMB(_object_particle_pool, struct object_particle, PARTICLE_COUNT_MAX,
    sizeof(struct object_particle));
MEMB(_collider_pool, struct collider, PARTICLE_COUNT_MAX,
    sizeof(struct collider));
MEMB(_rigid_body_pool, struct rigid_body, PARTICLE_COUNT_MAX,
    sizeof(struct rigid_body));
MEMB(_color_list_pool, color_rgb555_t, PARTICLE_COUNT_MAX,
    sizeof(color_rgb555_t));

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
component_particle_init(struct component *this)
{
        assert((COMPONENT_THIS(particle, ttl) > PARTICLE_TTL_MIN) &&
               (COMPONENT_THIS(particle, ttl) <= PARTICLE_TTL_MAX));
        assert(COMPONENT_THIS(particle, max_count) <= PARTICLE_COUNT_MAX);
        assert(COMPONENT_THIS(particle, emmission_count) <= COMPONENT_THIS(particle, max_count));

        particles_init();

        struct object_particle **object_particle_list;
        object_particle_list = &COMPONENT_THIS_PRIVATE_DATA(particle,
            object_particle_list)[0];
        uint32_t object_particle_count;
        object_particle_count = COMPONENT_THIS(particle, max_count);

        COMPONENT_THIS_PRIVATE_DATA(particle, object_particle_count) =
            object_particle_count;

        uint32_t object_idx;
        for (object_idx = 0; object_idx < object_particle_count; object_idx++) {
                struct object_particle *object_particle;
                object_particle = particles_particle_alloc();

                OBJECT(object_particle, active) = false;

                OBJECT_PRIVATE_DATA(object_particle, ttl) =
                    COMPONENT_THIS(particle, ttl);
                OBJECT_PRIVATE_DATA(object_particle, color_from).raw =
                    COMPONENT_THIS(particle, color_from).raw;
                OBJECT_PRIVATE_DATA(object_particle, color_to).raw =
                    COMPONENT_THIS(particle, color_to).raw;
                OBJECT_PRIVATE_DATA(object_particle, delta).x = F16(0.0f);
                OBJECT_PRIVATE_DATA(object_particle, delta).y = F16(0.0f);

                OBJECT_INIT(object_particle);

                object_particle_list[object_idx] = object_particle;
        }
}

void
component_particle_update(struct component *this)
{
        const struct object *object;
        object = COMPONENT_THIS(particle, object);

        bool looping;
        looping = COMPONENT_THIS(particle, looping);

        uint32_t emmission_count;
        emmission_count = COMPONENT_THIS(particle, emmission_count);

        struct object_particle **object_particle_list;
        object_particle_list = &COMPONENT_THIS_PRIVATE_DATA(particle,
            object_particle_list)[0];

        uint32_t object_idx;
        for (object_idx = 0; object_idx < emmission_count; object_idx++) {
                struct object_particle *object_particle;
                object_particle = object_particle_list[object_idx];

                /* Remove "dead" particles from the objects tree iff
                 * looping in allowed */
                if (looping &&
                    (OBJECT_PRIVATE_DATA(object_particle, ttl) == 0)) {
                        objects_object_remove((struct object *)object_particle);
                }

                if (objects_object_added((const struct object *)object_particle)) {
                        continue;
                }

                OBJECT(object_particle, active) = true;

                struct transform *transform;
                transform = &OBJECT_COMPONENT(object_particle, transform);

                COMPONENT(transform, position).z = F16(-1.0f);

                /* Reset TTL */
                OBJECT_PRIVATE_DATA(object_particle, ttl) =
                    COMPONENT_THIS(particle, ttl);

                objects_object_child_add((struct object *)object,
                    (struct object *)object_particle);
        }
}

static void
particles_init(void)
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

static struct object_particle *
particles_particle_alloc(void)
{
        static uint32_t id_offset = 0;

        assert(_initialized);

        struct object_particle *object_particle;
        object_particle =
            (struct object_particle *)memb_alloc(&_object_particle_pool);
        assert(object_particle != NULL);

        struct transform *transform;
        transform = &OBJECT_COMPONENT(object_particle, transform);

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

        color_rgb555_t *color_list;
        color_list = (color_rgb555_t *)memb_alloc(&_color_list_pool);
        assert(color_list != NULL);

        color_list[0].r = 31;
        color_list[0].g = 31;
        color_list[0].b = 31;

        OBJECT(object_particle, active) = false;
        OBJECT(object_particle, id) = OBJECT_ID_RESERVED_BEGIN + id_offset;
        id_offset++;
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
        OBJECT(object_particle, on_init) = object_particle_on_init;
        OBJECT(object_particle, on_update) = object_particle_on_update;
        OBJECT(object_particle, on_draw) = object_particle_on_draw;
        OBJECT(object_particle, on_destroy) = NULL;
        OBJECT(object_particle, on_collision) = NULL;
        OBJECT(object_particle, on_trigger) = NULL;

        OBJECT(object_particle, initialized) = false;
        OBJECT(object_particle, context) = NULL;

        /* Public data */
        OBJECT_PRIVATE_DATA(object_particle, ttl) = PARTICLE_TTL_MAX;

        OBJECT_PRIVATE_DATA(object_particle, color_from).r = 255;
        OBJECT_PRIVATE_DATA(object_particle, color_from).g = 255;
        OBJECT_PRIVATE_DATA(object_particle, color_from).b = 255;

        OBJECT_PRIVATE_DATA(object_particle, color_to).r = 255;
        OBJECT_PRIVATE_DATA(object_particle, color_to).g = 255;
        OBJECT_PRIVATE_DATA(object_particle, color_to).b = 255;

        OBJECT_PRIVATE_DATA(object_particle, delta).x = F16(0.0f);
        OBJECT_PRIVATE_DATA(object_particle, delta).y = F16(0.0f);

        color_rgb555_t *rgb555_table;
        rgb555_table = &OBJECT_PRIVATE_DATA(object_particle, rgb555_table)[0];

        uint32_t range;
        for (range = 0; range < PARTICLE_TTL_MAX; range++) {
                rgb555_table[range].raw = 0x0000;
        }

        return object_particle;
}

static void
particles_particle_free(struct object_particle *object_particle)
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

        color_rgb555_t *color_list;
        color_list = OBJECT(object_particle, color_list);
        assert(color_list != NULL);
        memb_free(&_color_list_pool, color_list);

        assert(object_particle != NULL);
        OBJECT(object_particle, active) = false;
        memb_free(&_object_particle_pool, object_particle);
}

static void
object_particle_on_init(struct object *this)
{
        /* We want a particle that's alive (TTL > 0) */
        assert((THIS_PRIVATE_DATA(object_particle, ttl) > PARTICLE_TTL_MIN) &&
               (THIS_PRIVATE_DATA(object_particle, ttl) <= PARTICLE_TTL_MAX));

        /* Compute table of color HSV values (very slow) */
        color_fix16_hsv_t hsv_from;
        color_rgb888_hsv_convert(
                &THIS_PRIVATE_DATA(object_particle, color_from), &hsv_from);

        color_fix16_hsv_t hsv_to;
        color_rgb888_hsv_convert(&THIS_PRIVATE_DATA(object_particle, color_to),
            &hsv_to);

        color_rgb555_t *rgb555_table;
        rgb555_table = &THIS_PRIVATE_DATA(object_particle, rgb555_table)[0];

        int16_t ttl;
        ttl = THIS_PRIVATE_DATA(object_particle, ttl);

        int16_t step;
        step = 256 / PARTICLE_TTL_LENGTH; /* Linear interpolation: [0..255] */
        /* Because our TTL goes from [PARTICLE_TTL_MAX..0], we want to
         * have the table reversed. Otherwise, we'll go from "color to"
         * to "color from". */
        int16_t range;
        for (range = 0; range <= ttl; range++) {
                color_fix16_hsv_t hsv;
                color_hsv_lerp8(&hsv_from, &hsv_to, range * step, &hsv);

                /* Compute table of HSV to RGB mapping */
                color_hsv_rgb555_convert(&hsv,
                    &rgb555_table[(PARTICLE_TTL_LENGTH - 1) - range]);
        }

        /* Set to the current TTL color */
        color_rgb555_t *color_list;
        color_list = THIS(object_particle, color_list);
        color_list[0].r = rgb555_table[ttl].r;
        color_list[0].g = rgb555_table[ttl].g;
        color_list[0].b = rgb555_table[ttl].b;
}

static void
object_particle_on_update(struct object *this)
{
        color_rgb555_t *rgb555_table;
        rgb555_table = &THIS_PRIVATE_DATA(object_particle, rgb555_table)[0];

        int16_t ttl;
        ttl = THIS_PRIVATE_DATA(object_particle, ttl);

        /* Set color to primitive */
        color_rgb555_t *color_list;
        color_list = THIS(object_particle, color_list);
        color_list[0].r = rgb555_table[ttl].r;
        color_list[0].g = rgb555_table[ttl].g;
        color_list[0].b = rgb555_table[ttl].b;

        if ((THIS_PRIVATE_DATA(object_particle, ttl) - 1) >= 0) {
                THIS_PRIVATE_DATA(object_particle, ttl)--;
        }

        struct transform *transform;
        transform = &THIS_COMPONENT(object_particle, transform);

        fix16_vector2_add((fix16_vector2_t *)&COMPONENT(transform, position),
            (fix16_vector2_t *)&COMPONENT(transform, position),
            &THIS_PRIVATE_DATA(object_particle, delta));
}

static void
object_particle_on_draw(struct object *this __unused)
{
}
