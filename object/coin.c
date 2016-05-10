/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

static struct transform _transform = {
        .active = true,
        .id = COMPONENT_ID_TRANSFORM,
        .object = NULL,
        .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 1.0f)
};

static struct collider _collider = {
        .active = true,
        .id = COMPONENT_ID_COLLIDER,
        .object = NULL,
        .width = 8,
        .height = 8,
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
        .object = NULL,
        .width = 8,
        .height = 8,
        .material = {
                .pseudo_trans = true,
                .solid_color = PALETTE_COLOR_16
        },
        .on_init = &component_sprite_on_init,
        .on_update = &component_sprite_on_update,
        .on_draw = component_sprite_on_draw,
        .on_destroy = NULL
};

const struct object object_coin = {
        .active = false,
        .id = OBJECT_ID_COIN,
        .component_list = {
                OBJECT_COMPONENT_INITIALIZER(transform, &_transform),
                OBJECT_COMPONENT_INITIALIZER(sprite, &_sprite),
                OBJECT_COMPONENT_INITIALIZER(collider, &_collider)
         },
        .component_count = 3
};
