/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_PARTICLE_H
#define ENGINE_PARTICLE_H

#include <inttypes.h>

#include "object.h"

#define PARTICLE_COUNT_MAX      128

#define PARTICLE_TTL_MIN        0
#define PARTICLE_TTL_MAX        31
#define PARTICLE_TTL_LENGTH     (PARTICLE_TTL_MAX + 1)

struct object_particle;

struct particle {
        COMPONENT_DECLARATIONS

        bool looping;
        uint32_t max_count;
        uint32_t emmission_count;
        int16_t ttl;
        color_rgb888_t color_from;
        color_rgb888_t color_to;

        /* Private data */
        struct {
                uint32_t m_state;
                uint32_t m_last_state;

                struct object_particle *m_object_particle_list[PARTICLE_COUNT_MAX];
                uint32_t m_object_particle_count;
        } private_data;
};

struct object_particle {
        OBJECT_DECLARATIONS

        /* Public data */
        struct {
        } data;

        /* Private data */
        struct {
                int16_t m_ttl;
                color_rgb888_t m_color_from;
                color_rgb888_t m_color_to;
                fix16_vector2_t m_delta;
                color_rgb555_t m_rgb555_table[PARTICLE_TTL_LENGTH];
        } private_data;
} __aligned (256);

extern void component_particle_init(struct component *);
extern void component_particle_update(struct component *);

#endif /* !ENGINE_PARTICLE_H */
