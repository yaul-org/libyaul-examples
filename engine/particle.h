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

struct object_particle {
        OBJECT_DECLARATIONS

        /* Public data */
        struct {
        } functions;

        struct {
                int16_t m_ttl;
                color_rgb555_t m_color_from;
                color_rgb555_t m_color_to;
                fix16_vector2_t m_delta;
        } data;

        struct {
                fix16_vector3_t m_orig_pos;
        } private_data;
} __aligned(128);

extern void particle_init(void);
extern struct object_particle *particle_alloc(void);
extern void particle_free(struct object_particle *);

#endif /* !ENGINE_PARTICLE_H */
