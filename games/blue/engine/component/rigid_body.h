/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_RIGID_BODY_H
#define ENGINE_RIGID_BODY_H

#include "../engine.h"

#define RIGID_BODY_FORCES_MAX 4

struct rigid_body {
        COMPONENT_DECLARATIONS

        bool kinematic;

        void (*forces_add)(struct rigid_body *, const fix16_vector2_t *);
        void (*forces_clear)(struct rigid_body *);
        void (*forces_sum)(struct rigid_body *, fix16_vector2_t *);

        /* Private data */
        fix16_vector2_t _displacement;
        fix16_vector2_t _velocity;
        fix16_vector2_t _acceleration;
        fix16_t _mass;
        /* An extra force for gravity */
        fix16_vector2_t _forces[RIGID_BODY_FORCES_MAX];
        uint32_t _forces_cnt;
} __aligned (64);

extern void component_rigid_body_init(struct component *);

#endif /* !ENGINE_RIGID_BODY_H */
