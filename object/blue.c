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
        .object = (struct object *)&object_blue,
        .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 7.0f)
};

static struct collider _collider = {
        .active = true,
        .id = COMPONENT_ID_COLLIDER,
        .object = (const struct object *)&object_blue,
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

static struct rigid_body _rigid_body = {
        .active = true,
        .id = COMPONENT_ID_RIGID_BODY,
        .object = (const struct object *)&object_blue,
        .on_init = &component_collider_on_init,
        .on_update = NULL,
        .on_draw = NULL,
        .on_destroy = NULL
};

static struct sprite _sprite = {
        .active = true,
        .id = COMPONENT_ID_SPRITE,
        .object = (const struct object *)&object_blue,
        .visible = true,
        .width = 8,
        .height = 16,
        .material = {
                .pseudo_trans = false,
                .solid_color = COLOR_RGB555_INITIALIZER(0, 0, 31)
        },
        .on_init = &component_sprite_on_init,
        .on_update = &component_sprite_on_update,
        .on_draw = component_sprite_on_draw,
        .on_destroy = NULL
};

static struct blue_mgr _blue_mgr = {
        .active = true,
        .id = COMPONENT_ID_BLUE_MGR,
        .object = (struct object *)&object_blue,
        .start_position = FIX16_VECTOR2_INITIALIZER(0.0f, 0.0f),
        .on_init = &component_blue_mgr_on_init,
        .on_update = &component_blue_mgr_on_update,
        .on_draw = &component_blue_mgr_on_draw,
        .on_destroy = &component_blue_mgr_on_destroy
};

struct object object_blue = {
        .active = true,
        .id = OBJECT_ID_BLUE,
        .component_list = {
                /* Must be the first component */
                OBJECT_COMPONENT_INITIALIZER(transform, &_transform),
                OBJECT_COMPONENT_INITIALIZER(sprite, &_sprite),
                OBJECT_COMPONENT_INITIALIZER(collider, &_collider),
                OBJECT_COMPONENT_INITIALIZER(rigid_body, &_rigid_body),
                OBJECT_COMPONENT_INITIALIZER(blue_mgr, &_blue_mgr)
        },
        .component_count = 5
};
