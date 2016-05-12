/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

void
component_layer_on_init(struct component *this __unused)
{
        assert(THIS(layer, visible));
        assert((THIS(layer, number) >= 0) && (THIS(layer, number) <= 3));
}

void
component_layer_on_update(struct component *this __unused)
{
}

void
component_layer_on_draw(struct component *this __unused)
{
}

void
component_layer_on_destroy(struct component *this __unused)
{
}
