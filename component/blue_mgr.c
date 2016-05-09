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
        cons_buffer("Hello from component blue_mgr\n");
}

void
component_blue_mgr_on_draw(struct component *this __unused)
{
}

void
component_blue_mgr_on_destroy(struct component *this __unused)
{
}
