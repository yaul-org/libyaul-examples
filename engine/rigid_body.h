/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_RIGID_BODY_H
#define ENGINE_RIGID_BODY_H

#include <yaul.h>

#include <inttypes.h>
#include <math.h>

#include "component.h"

#define RIGID_BODY_FORCES_MAX 4

struct rigid_body {
        COMPONENT_DECLARATIONS

        bool kinematic;

        struct {
                void (*m_forces_add)(struct rigid_body *,
                    const fix16_vector2_t *);
                void (*m_forces_clear)(struct rigid_body *);
                void (*m_forces_sum)(struct rigid_body *, fix16_vector2_t *);
        } functions;

        struct {
                fix16_vector2_t m_displacement;
                fix16_vector2_t m_velocity;
                fix16_vector2_t m_acceleration;
                fix16_t m_mass;
                /* An extra force for gravity */
                fix16_vector2_t m_forces[RIGID_BODY_FORCES_MAX];
                uint32_t m_forces_cnt;
        } private_data;
} __aligned (64);

extern void component_rigid_body_init(struct component *);

#endif /* !ENGINE_RIGID_BODY_H */
