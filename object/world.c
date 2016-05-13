/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

static struct transform _transform = {
        .active = true,
        .id = COMPONENT_ID_TRANSFORM,
        .object = (struct object *)&object_world,
        .position = FIX16_VECTOR3_INITIALIZER(0.0f, 0.0f, 7.0f)
};

static struct coin_mgr _coin_mgr = {
        .active = true,
        .id = COMPONENT_ID_COIN_MGR,
        .object = (struct object *)&object_world,
        .on_init = &component_coin_mgr_on_init,
        .on_update = &component_coin_mgr_on_update,
        .on_draw = &component_coin_mgr_on_draw,
        .on_destroy = &component_coin_mgr_on_destroy,
        .coins = 64,
        .functions = {
                .m_spawn = &component_coin_mgr_spawn
        }
};

static struct world_mgr _world_mgr = {
        .active = true,
        .id = COMPONENT_ID_WORLD_MGR,
        .object = (struct object *)&object_world,
        .on_init = &component_world_mgr_on_init,
        .on_update = &component_world_mgr_on_update,
        .on_draw = &component_world_mgr_on_draw,
        .on_destroy = &component_world_mgr_on_destroy,
        .world = 0
};

static struct layer _layer = {
        .active = true,
        .id = COMPONENT_ID_LAYER,
        .object = (struct object *)&object_world,
        .on_init = &component_layer_on_init,
        .on_update = &component_layer_on_update,
        .on_draw = &component_layer_on_draw,
        .on_destroy = &component_layer_on_destroy,
        .visible = true,
        .transparent = true,
        .number = 0,
        .scroll = FIX16_VECTOR2_INITIALIZER(0.0f, 0.0f),
        .palette = NULL,
        .char_base = NULL,
        .functions = {
                .m_map = component_layer_map
        }
};

struct object object_world = {
        .active = true,
        .id = OBJECT_ID_WORLD,
        .component_list = {
                OBJECT_COMPONENT_INITIALIZER(transform, &_transform),
                OBJECT_COMPONENT_INITIALIZER(coin_mgr, &_coin_mgr),
                OBJECT_COMPONENT_INITIALIZER(world_mgr, &_world_mgr),
                OBJECT_COMPONENT_INITIALIZER(layer, &_layer)
        },
        .component_count = 4
};
