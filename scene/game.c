/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

void
scene_game_init(struct scene_ctx *scene_ctx __unused)
{
        object_init(&object_camera);
        object_init(&object_blue);
        object_init(&object_world);

        objects_object_add(&object_camera);
        objects_object_add(&object_blue);
        objects_object_add(&object_world);
}

void
scene_game_update(struct scene_ctx *scene_ctx __unused)
{
}

void
scene_game_draw(struct scene_ctx *scene_ctx __unused)
{
}

void
scene_game_exit(struct scene_ctx *scene_ctx __unused)
{
        objects_clear();
}
