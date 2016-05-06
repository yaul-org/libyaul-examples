/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_COLLIDER_H
#define ENGINE_COLLIDER_H

#include <yaul.h>

#include "object.h"
#include "component.h"
#include "aabb.h"

struct collider {
        COMPONENT_DECLARATIONS

        int16_t width;
        int16_t height;
        bool trigger;
        bool fixed;
        bool show;

        struct {
                struct aabb m_aabb;

                struct {
                        struct object object;
                        fix16_vector3_t vertex_list[4];
                        color_rgb555_t color_list[1];
                } m_show;
        } private_data;
} __aligned (32);

struct collider_info {
        int16_t overlap;
        int16_vector2_t direction;
};

extern void component_collider_on_init(struct component *);

#endif /* !ENGINE_COLLIDER_H */
