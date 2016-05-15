/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

static struct sprite _sprite = {
        .active = true,
        .id = COMPONENT_ID_SPRITE,
        .object = NULL,
        .visible = true,
        .width = 0,
        .height = 0,
        .material = {
                .pseudo_trans = false,
                .solid_color = COLOR_RGB555_INITIALIZER(0, 0, 0)
        },
        .on_init = &component_sprite_on_init,
        .on_update = &component_sprite_on_update,
        .on_draw = component_sprite_on_draw,
        .on_destroy = NULL
};

void
component_camera_on_init(struct component *this __unused)
{
        COMPONENT(&_sprite, object) = NULL;
        COMPONENT(&_sprite, width) = THIS(camera, width);
        COMPONENT(&_sprite, height) = THIS(camera, height);

        object_component_add((struct object *)THIS(camera, object),
            (struct component *)&_sprite, sizeof(struct sprite));
}

void
component_camera_on_update(struct component *this __unused)
{
}

void
component_camera_on(struct component *this __unused)
{
        struct sprite *sprite;
        sprite = (struct sprite *)object_component_find(THIS(camera, object),
            COMPONENT_ID_SPRITE);
        assert(sprite != NULL);

        COMPONENT(sprite, active) = false;
}

void
component_camera_off(struct component *this __unused)
{
        struct sprite *sprite;
        sprite = (struct sprite *)object_component_find(THIS(camera, object),
            COMPONENT_ID_SPRITE);
        assert(sprite != NULL);

        COMPONENT(sprite, active) = true;
}
