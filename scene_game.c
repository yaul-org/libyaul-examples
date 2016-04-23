/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

#include "object_camera.h"
#include "object_world.h"
#include "object_blue.h"

static struct {
} _scene_game __unused;

void
scene_game_init(struct scene_ctx *scene_ctx __unused)
{
        objects_object_add((struct object *)&object_camera);
        objects_object_add((struct object *)&object_world);
        objects_object_add((struct object *)&object_blue);
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
