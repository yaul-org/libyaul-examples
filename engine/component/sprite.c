/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

void
component_sprite_on_init(struct component *this __unused)
{
        assert(((THIS(sprite, width) >= 8) && (THIS(sprite, width) <= 256)) &&
               ((THIS(sprite, height) >= 8) && (THIS(sprite, height) <= 256)));
}

void
component_sprite_on_update(struct component *this __unused)
{
        cons_buffer("Hello from component sprite\n");
}

void
component_sprite_on_draw(struct component *this __unused)
{
}
