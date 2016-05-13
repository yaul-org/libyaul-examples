/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

static struct layer_map _layer_maps[LAYERS_MAX];

void
component_layer_on_init(struct component *this __unused)
{
        assert(THIS(layer, visible));
        assert((THIS(layer, number) >= 0) && (THIS(layer, number) <= 3));

        /* Allocate layer map */
        struct layer_map *layer_map;
        layer_map = &_layer_maps[THIS(layer, number)];

        /* Clear page pointers to every plane */
        (void)memset(&layer_map->plane[0], 0x00, 4 * sizeof(uint16_t *));
        (void)memset(&layer_map->plane[1], 0x00, 4 * sizeof(uint16_t *));
        (void)memset(&layer_map->plane[2], 0x00, 4 * sizeof(uint16_t *));
        (void)memset(&layer_map->plane[3], 0x00, 4 * sizeof(uint16_t *));

        /* Make sure the layer is registered */
        assert(layers_layer_registered(THIS(layer, number)));

        layer_map->plane[0].page[0] = (uint16_t *)memb_alloc(&_page_pool);
}

void
component_layer_on_update(struct component *this __unused)
{
}

void
component_layer_on_draw(struct component *this __unused)
{
}

void
component_layer_on_destroy(struct component *this __unused)
{
}

struct layer_map *
component_layer_map(struct component *this __unused)
{
        return &_layer_maps[THIS(layer, number)];
}
