/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

static struct {
} scene_splash;

void
scene_splash_init(struct scene_ctx *scene_ctx)
{
        scene_ctx->sc_data = &scene_splash;
}

void
scene_splash_update(struct scene_ctx *scene_ctx __unused)
{
}

void
scene_splash_draw(struct scene_ctx *scene_ctx __unused)
{
}

void
scene_splash_exit(struct scene_ctx *scene_ctx __unused)
{
}
