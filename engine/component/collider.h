/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_COLLIDER_H
#define ENGINE_COLLIDER_H

#include "../engine.h"

struct collider {
        COMPONENT_DECLARATIONS

        int16_t width;
        int16_t height;
        bool trigger;
        bool fixed;

        /* Private data */
        struct aabb _aabb;
} __aligned (64);

struct collider_info {
        int16_t overlap;
        int16_vector2_t direction;
};

extern void component_collider_on_init(struct component *);

#endif /* !ENGINE_COLLIDER_H */
