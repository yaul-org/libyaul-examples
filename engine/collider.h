/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_COLLIDER_H
#define ENGINE_COLLIDER_H

#include "component.h"
#include "aabb.h"

struct collider {
        COMPONENT_DECLARATIONS

        bool trigger;
        bool fixed;
        struct aabb aabb;
} __aligned(32);

struct collider_info {
        int16_t overlap;
        int16_vector2_t direction;
};

void collider_init(struct collider *, uint32_t, uint16_t, uint16_t, bool, bool);

#endif /* !ENGINE_COLLIDER_H */
