/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_SPRITE_H
#define ENGINE_SPRITE_H

#include "../engine.h"

struct sprite {
        COMPONENT_DECLARATIONS

        uint32_t width;
        uint32_t height;

        struct material material;
} __aligned (64);

extern void component_sprite_on_init(struct component *);
extern void component_sprite_on_update(struct component *);
extern void component_sprite_on_draw(struct component *);

#endif /* !ENGINE_SPRITE_H */
