/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_LAYER_H
#define ENGINE_LAYER_H

#include "../engine.h"

struct layer {
        COMPONENT_DECLARATIONS

        bool visible;
        bool transparent;
        int16_t number;
        fix16_vector2_t scroll;
        color_rgb555_t *palette;
        uint16_t *char_base;

        struct {
                struct layer_map *(*m_map)(struct component *);
        } functions;
} __aligned (64);

extern void component_layer_on_init(struct component *);
extern void component_layer_on_update(struct component *);
extern void component_layer_on_draw(struct component *);
extern void component_layer_on_destroy(struct component *);

extern struct layer_map *component_layer_map(struct component *);

#endif /* !ENGINE_LAYER_H */
