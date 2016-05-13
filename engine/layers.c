/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "engine.h"

static bool _initialized = false;

static struct {
        bool enabled;
        struct object *object;
        const struct scrn_cell_format *format;
} _layer[LAYERS_MAX];

static const uint8_t _layer_number_scrn[] = {
        SCRN_NBG0,
        SCRN_NBG1,
        SCRN_NBG2,
        SCRN_NBG3
};

/*
 * Initialize layers system.
 */
void
layers_init(void)
{
        if (_initialized) {
                return;
        }

        uint32_t layer_idx;
        for (layer_idx = 0; layer_idx < LAYERS_MAX; layer_idx++) {
                _layer[layer_idx].enabled = false;
                _layer[layer_idx].format = NULL;
        }

        _initialized = true;
}

void
layers_update(void)
{
        const struct objects *objects;
        objects = objects_fetch();

        uint32_t object_idx;
        for (object_idx = 0; objects->list[object_idx] != NULL; object_idx++) {
                struct object *object;
                object = (struct object *)objects->list[object_idx];

                struct layer *layer;
                layer = (struct layer *)object_component_find(object,
                    COMPONENT_ID_LAYER);
                if ((layer == NULL) || !COMPONENT(layer, active)) {
                        continue;
                }
        }
}

void
layers_draw(void)
{
        const struct objects *objects;
        objects = objects_fetch();

        uint32_t object_idx;
        for (object_idx = 0; objects->list[object_idx] != NULL; object_idx++) {
                struct object *object;
                object = (struct object *)objects->list[object_idx];

                struct layer *layer;
                layer = (struct layer *)object_component_find(object,
                    COMPONENT_ID_LAYER);
                if ((layer == NULL) || !COMPONENT(layer, active)) {
                        continue;
                }

                /* Layer must be enabled */
                assert(_layer[COMPONENT(layer, number)].enabled);

                uint8_t scrn;
                scrn = _layer_number_scrn[COMPONENT(layer, number)];

                /* Display */
                if (COMPONENT(layer, visible)) {
                        vdp2_scrn_display_set(scrn,
                            COMPONENT(layer, transparent));
                } else {
                        vdp2_scrn_display_unset(scrn);
                        /* No need to continue if the background is not
                         * displayed */
                        continue;
                }

                /* Scrolling */
                vdp2_scrn_scroll_x_set(scrn, COMPONENT(layer, scroll).x);
                vdp2_scrn_scroll_y_set(scrn, COMPONENT(layer, scroll).y);
        }
}

void
layers_layer_register(int32_t number)
{
        assert((number >= 0) && (number <= 3));

        uint8_t scrn;
        scrn = _layer_number_scrn[number];

        /* Make sure the layer isn't already registered */
        assert(!_layer[number].enabled);

        _layer[number].enabled = true;
        _layer[number].format = vdp2_scrn_cell_format_get(scrn);
}

void
layers_layer_unregister(int32_t number)
{
        assert((number >= 0) && (number <= 3));

        /* Make sure the layer isn't already registered */
        assert(_layer[number].enabled);

        _layer[number].enabled = false;
        _layer[number].object = NULL;
        _layer[number].format = NULL;
}

bool
layers_layer_registered(int32_t number)
{
        assert((number >= 0) && (number <= 3));

        return (_layer[number].enabled && (_layer[number].format != NULL));
}
