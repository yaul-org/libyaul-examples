/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

void
component_coin_on_init(struct component *this)
{
        THIS(coin, ttl) = 0;
}

void
component_coin_on_update(struct component *this __unused)
{
        struct sprite *sprite;
        sprite = (struct sprite *)object_component_find(
                THIS(coin, object), COMPONENT_ID_SPRITE);

        if (COMPONENT(sprite, visible)) {
                THIS(coin, ttl)++;
        }
}

void
component_coin_on_draw(struct component *this __unused)
{
}

void
component_coin_on_destroy(struct component *this __unused)
{
}
