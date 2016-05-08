/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "coin.h"

#include "object_coin.h"

#define OBJECT_COIN_INITIALIZER                                                \
        .active = false,                                                       \
        .id = OBJECT_ID_COIN,                                                  \
        .component_list = {                                                    \
                /* Must be the first component */                              \
                (struct component *)&_transform,                               \
                (struct component *)&_sprite,                                  \
                (struct component *)&_collider                                 \
         },                                                                    \
        .component_count = 3

static struct transform _transform = {
        .active = true,
        .id = COMPONENT_ID_TRANSFORM,
        .object = (struct object *)&object_coin,
        .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 2.0f)
};

static struct collider _collider = {
        .active = true,
        .id = COMPONENT_ID_COLLIDER,
        .object = (const struct object *)&object_coin,
        .width = 8,
        .height = 16,
        .trigger = false,
        .fixed = false,
        .show = true,
        .on_init = &component_collider_on_init,
        .on_update = NULL,
        .on_draw = NULL,
        .on_destroy = NULL
};

static struct sprite _sprite = {
        .active = true,
        .id = COMPONENT_ID_SPRITE,
        .object = (const struct object *)&object_coin,
        .width = 8,
        .height = 16,
        .material = {
                .pseudo_trans = true,
                .solid_color = blue_palette[16]
        },
        .on_init = &component_sprite_on_init,
        .on_update = &component_sprite_on_update,
        .on_draw = component_sprite_on_draw,
        .on_destroy = NULL
};
