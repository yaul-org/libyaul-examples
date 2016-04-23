/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

static struct {
} scene_title;

void
scene_title_init(struct scene_ctx *scene_ctx)
{
        scene_ctx->sc_data = &scene_title;
}

void
scene_title_update(struct scene_ctx *scene_ctx __unused)
{
}

void
scene_title_draw(struct scene_ctx *scene_ctx __unused)
{
}

void
scene_title_exit(struct scene_ctx *scene_ctx __unused)
{
}
