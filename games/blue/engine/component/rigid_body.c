/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

static void forces_add(struct rigid_body *, const fix16_vector2_t *);
static void forces_clear(struct rigid_body *);
static void forces_sum(struct rigid_body *, fix16_vector2_t *);

void
component_rigid_body_on_init(struct component *this)
{
        THIS_FUNCTION(rigid_body, forces_add) = forces_add;
        THIS_FUNCTION(rigid_body, forces_clear) = forces_clear;
        THIS_FUNCTION(rigid_body, forces_sum) = forces_sum;

        THIS_P_DATA(rigid_body, mass) = F16(1.0f);

        fix16_vector2_zero(&THIS_P_DATA(rigid_body, displacement));
        fix16_vector2_zero(&THIS_P_DATA(rigid_body, velocity));
        fix16_vector2_zero(&THIS_P_DATA(rigid_body, acceleration));

        memset(&THIS_P_DATA(rigid_body, forces), 0x00,
            RIGID_BODY_FORCES_MAX * sizeof(fix16_vector2_t));
        THIS_P_DATA(rigid_body, forces_cnt) = 0;
}

static void
forces_add(struct rigid_body *this, const fix16_vector2_t *add_force)
{
        uint32_t forces_cnt;
        forces_cnt = THIS_P_DATA(rigid_body, forces_cnt);

        assert(forces_cnt < RIGID_BODY_FORCES_MAX);

        fix16_vector2_dup(add_force,
            &THIS_P_DATA(rigid_body, forces)[forces_cnt]);

        THIS_P_DATA(rigid_body, forces_cnt)++;
}

static void
forces_clear(struct rigid_body *this)
{
        THIS_P_DATA(rigid_body, forces_cnt) = 0;
}

static void
forces_sum(struct rigid_body *this, fix16_vector2_t *result)
{
        fix16_vector2_zero(result);

        uint32_t forces_cnt;
        forces_cnt = THIS_P_DATA(rigid_body, forces_cnt);

        uint32_t force_idx;
        for (force_idx = 0; force_idx < forces_cnt; force_idx++) {
                fix16_vector2_t *force;
                force = &THIS_P_DATA(rigid_body, forces)[force_idx];

                fix16_vector2_add(result, force, result);
        }
}
