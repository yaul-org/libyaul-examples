/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_LAYERS_H
#define ENGINE_LAYERS_H

#include "engine.h"

#define LAYERS_MAX 4

struct layer_map {
        struct {
                uint16_t page[32 * 32];
        } plane[2]; /* Plane A and B only */
};

extern void layers_init(void);
extern void layers_update(void);
extern void layers_draw(void);

#endif /* !ENGINE_LAYERS_H */
