/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../../engine.h"

void
component_camera_on_init(struct component *this __unused)
{
}

void
component_camera_on_update(struct component *this __unused)
{
        cons_buffer("Hello from component camera\n");
}
