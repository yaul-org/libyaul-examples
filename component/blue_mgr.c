/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

void
component_blue_mgr_on_init(struct component *this __unused)
{
}

void
component_blue_mgr_on_update(struct component *this __unused)
{
        struct transform *transform;
        transform = (struct transform *)object_component_find(
                THIS(camera_mgr, object), COMPONENT_ID_TRANSFORM);

        COMPONENT(transform, position).x = THIS(blue_mgr, start_position).x;
        COMPONENT(transform, position).y = THIS(blue_mgr, start_position).y;
}

void
component_blue_mgr_on_draw(struct component *this __unused)
{
}

void
component_blue_mgr_on_destroy(struct component *this __unused)
{
}
