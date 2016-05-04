/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include "common.h"
#include "collider.h"

void
component_collider_init(struct component *this)
{
        int16_t width;
        width = C_THIS(collider, width);

        int16_t height;
        height = C_THIS(collider, height);

        struct aabb *aabb;
        aabb = &C_THIS_P_DATA(collider, aabb);

        aabb->center.x = width / 2;
        aabb->center.y = height / 2;
        aabb->min.x = 0;
        aabb->min.y = 0;
        aabb->max.x = width - 1;
        aabb->max.y = height - 1;
        aabb->half.x = width / 2;
        aabb->half.y = height / 2;
}
