/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "engine.h"

static bool _initialized = false;

static const uint8_t _layer_number_scrn[] = {
        SCRN_NBG0,
        SCRN_NBG1,
        SCRN_NBG2,
        SCRN_NBG3
};

static struct {
        uint8_t scrn;
        struct layer *layer;
} _cached_layers[LAYERS_MAX];

static struct {
        bool cp_uploaded;
        bool color_palette_uploaded;
} _layer_state[LAYERS_MAX];

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
                _layer_state[layer_idx].cp_uploaded = false;
                _layer_state[layer_idx].color_palette_uploaded = false;

                _cached_layers[layer_idx].scrn = 0;
                _cached_layers[layer_idx].layer = NULL;
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
                struct object *object __unused;
                object = (struct object *)objects->list[object_idx];

                struct layer *layer;
                layer = (struct layer *)object_component_find(object,
                    COMPONENT_ID_LAYER);

                if (layer == NULL) {
                        continue;
                }

                uint8_t scrn;
                scrn = _layer_number_scrn[COMPONENT(layer, number)];

                /* Reset cached layer and layer state in case the layer
                 * component was removed (i.e. the object was
                 * destroyed) */
                _layer_state[scrn].cp_uploaded = false;
                _layer_state[scrn].color_palette_uploaded = false;

                _cached_layers[scrn].scrn = 0;
                _cached_layers[scrn].layer = NULL;

                if (!COMPONENT(layer, active)) {
                        continue;
                }

                _cached_layers[scrn].scrn = 0;
                _cached_layers[scrn].layer = layer;
        }
}

void
layers_draw(void)
{
        uint32_t layer_idx;
        for (layer_idx = 0; layer_idx < LAYERS_MAX; layer_idx++) {
                struct layer *layer;
                layer = _cached_layers[layer_idx].layer;

                if (layer == NULL) {
                        continue;
                }

                const struct object *object;
                object = COMPONENT(layer, object);

                uint8_t scrn;
                scrn = _cached_layers[layer_idx].scrn;

                /* Display */
                if (COMPONENT(layer, visible)) {
                        vdp2_scrn_display_set(scrn,
                            COMPONENT(layer, transparent));
                } else {
                        vdp2_scrn_display_unset(scrn);
                        /* No need to continue if the background
                         * is not displayed */
                        continue;
                }

                const struct scrn_cell_format *cell_format;
                cell_format = vdp2_scrn_cell_format_get(scrn);

                if (!_layer_state[layer_idx].cp_uploaded) {
                        (void)memcpy((void *)cell_format->scf_cp_table,
                            (void *)COMPONENT(layer, character_pattern_base),
                            2048);

                        _layer_state[layer_idx].cp_uploaded = true;
                }

                if (!_layer_state[layer_idx].color_palette_uploaded) {
                        (void)memcpy((void *)cell_format->scf_color_palette,
                            (void *)&COMPONENT(layer, color_palette),
                            256 * sizeof(color_rgb555_t));

                        _layer_state[layer_idx].color_palette_uploaded = true;
                }

                /* Convert map */
                uint32_t page_width;
                page_width = SCRN_CALCULATE_PAGE_WIDTH(cell_format);
                uint32_t page_height;
                page_height = SCRN_CALCULATE_PAGE_HEIGHT(cell_format);
                uint32_t page_size __unused;
                page_size = SCRN_CALCULATE_PAGE_SIZE(cell_format);

                uint16_t *a_plane;
                uint16_t *b_plane __unused;
                uint16_t *c_plane __unused;
                uint16_t *d_plane __unused;
                a_plane = (uint16_t *)cell_format->scf_map.plane_a;
                b_plane = (uint16_t *)cell_format->scf_map.plane_b;
                c_plane = (uint16_t *)cell_format->scf_map.plane_c;
                d_plane = (uint16_t *)cell_format->scf_map.plane_d;

                /* 1x1 plane size */
                uint16_t *a_pages[4];
                a_pages[0] = &a_plane[0];
                a_pages[1] = NULL;
                a_pages[2] = NULL;
                a_pages[3] = NULL;

                uint32_t page_y;
                for (page_y = 0; page_y < page_height; page_y++) {
                        uint32_t page_x;
                        for (page_x = 0; page_x < page_width; page_x++) {
                                uint16_t page_idx;
                                page_idx = page_x + (page_width * page_y);

                                uint32_t value __unused;
                                value = COMPONENT(layer, map).plane[0].page[page_idx];

                                uint16_t pnd;
                                pnd = SCRN_PND_CONFIG_6(
                                        cell_format->scf_cp_table + (value * 256),
                                        cell_format->scf_color_palette,
                                        /* vf = */ 0,
                                        /* hf = */ 0);

                                a_pages[0][page_idx] = pnd;
                        }
                }

                /* Scrolling */
                struct transform *transform;
                transform = (struct transform *)object_component_find(object,
                    COMPONENT_ID_TRANSFORM);
                const fix16_vector3_t *position;
                position = &COMPONENT(transform, position);

                vdp2_scrn_scroll_x_set(scrn, position->x);
                vdp2_scrn_scroll_y_set(scrn, position->y);
        }
}
