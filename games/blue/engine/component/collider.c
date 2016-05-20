/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

void
component_collider_on_init(struct component *this)
{
        int16_t width;
        width = THIS(collider, width);

        int16_t height;
        height = THIS(collider, height);

        struct aabb *aabb;
        aabb = &THIS_P_DATA(collider, aabb);

        aabb->center.x = width / 2;
        aabb->center.y = height / 2;
        aabb->min.x = 0;
        aabb->min.y = 0;
        aabb->max.x = width - 1;
        aabb->max.y = height - 1;
        aabb->half.x = width / 2;
        aabb->half.y = height / 2;
}
