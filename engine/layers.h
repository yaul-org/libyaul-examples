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
                uint16_t *page[4];
        } plane[4];
};

extern void layers_init(void);
extern void layers_update(void);
extern void layers_draw(void);
extern void layers_layer_register(int32_t);
extern void layers_layer_unregister(int32_t);
extern bool layers_layer_registered(int32_t);

#endif /* !ENGINE_LAYERS_H */
