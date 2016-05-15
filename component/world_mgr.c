/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

/* Total number of colliders in a world */
#define COLLIDERS_MAX   128
/* Total number of coins in a world */
#define COINS_MAX       1024

/* World file format
 *
 *  0x0000          Header
 *  0x0028          Collider data
 *  0x0028 + x      Coin data
 *  0x0028 + x + y  Map data */

struct world_header {
        const char name[16];
        uint16_t width;
        uint16_t height;
        color_rgb555_t bg_color;
        fix16_t camera_speed;
        uint16_t start_delay;
        fix16_vector2_t blue_position;
        uint16_t collider_count;
        uint16_t coin_count;
} __packed;

struct world_collider {
        uint16_t type;
        fix16_vector2_t position;
        int16_vector2_t cell_position;
        uint16_t width;
        uint16_t height;
} __packed;

struct world_coin {
        struct {
                unsigned int :7;
                unsigned int coin:1;
        } __packed type;

        fix16_vector2_t position;
        int16_vector2_t cell_position;
} __packed;

struct world_column {
        struct {
                unsigned int :2;
                unsigned int trigger:1;
                unsigned int collider:1;
                unsigned int coin:1;
                unsigned int number:3;
        } __packed cell[14]; /* SCREEN_WIDTH / CELL_HEIGHT */
} __packed;

MEMB(_collider_pool, struct collider, COLLIDERS_MAX, sizeof(struct collider));

static struct coin_mgr *_coin_mgr;
static struct layer *_layer;
static void *_map_fh;
static struct world_header _map_header;
static struct world_collider _map_colliders[COLLIDERS_MAX];
static struct world_coin _map_coins[COINS_MAX];
static struct world_column _column[20];
static uint32_t _column_idx = 0;

static uint8_t _character_pattern_base[2048];

void
component_world_mgr_on_init(struct component *this __unused)
{
        assert((THIS(world_mgr, world) >= 0) &&
               (THIS(world_mgr, world) < BLUE_WORLDS));

        memb_init(&_collider_pool);

        _coin_mgr = (struct coin_mgr *)object_component_find(
                THIS(world_mgr, object), COMPONENT_ID_COIN_MGR);

        _layer = (struct layer *)object_component_find(
                THIS(world_mgr, object), COMPONENT_ID_LAYER);

        /* Open file */
        const char *world_filename;
        world_filename = blue_worlds[THIS(world_mgr, world)];

        _map_fh = fs_open(world_filename);
        assert(_map_fh != NULL);

        /* Read header */
        fs_read(_map_fh, &_map_header, sizeof(struct world_header));

        assert((_map_header.collider_count > 0) &&
               (_map_header.collider_count <= COLLIDERS_MAX));
        assert((_map_header.coin_count > 0) &&
               (_map_header.coin_count <= COINS_MAX));

        /* Read in collision data */
        fs_read(_map_fh, &_map_colliders[0],
            _map_header.collider_count * sizeof(struct world_collider));

        /* Read in coin data */
        fs_read(_map_fh, &_map_coins[0],
            _map_header.coin_count * sizeof(struct world_coin));

        /* Spawn all coins */
        uint32_t coin_idx;
        for (coin_idx = 0; coin_idx < _map_header.coin_count; coin_idx++) {
                struct world_coin *coin;
                coin = &_map_coins[coin_idx];

                COMPONENT_FUNCTION_CALL(_coin_mgr, spawn, &coin->position);
        }

        /* Spawn at position */
        struct blue_mgr *blue_mgr;
        blue_mgr = (struct blue_mgr *)objects_component_find(COMPONENT_ID_BLUE_MGR);
        assert(blue_mgr != NULL);
        COMPONENT(blue_mgr, start_position).x = _map_header.blue_position.x;
        COMPONENT(blue_mgr, start_position).y = _map_header.blue_position.y;

        /* Copy character pattern and color palette data */
        COMPONENT(_layer, character_pattern_base) = &_character_pattern_base[0];
        fs_texture_load(FS_LOAD_TEXTURE_1D, "/WORLD.TGA",
            COMPONENT(_layer, character_pattern_base),
            COMPONENT(_layer, color_palette));
}

void
component_world_mgr_on_update(struct component *this __unused)
{
        (void)sprintf(text_buffer, "Hello from component world_mgr\n\"%s\"\n",
            _map_header.name);
        cons_buffer(text_buffer);

        /* Calculate offset to actual map data */
        uint32_t file_offset;
        file_offset =
            /* Header */
            sizeof(struct world_header) +
            /* Colliders */
            (_map_header.collider_count * sizeof(struct world_collider)) +
            /* Coins */
            (_map_header.coin_count * sizeof(struct world_coin)) +
            /* Column */
            (0 * sizeof(struct world_column));
        fs_seek(_map_fh, file_offset, SEEK_SET);

        /* Determine if we're at the end of the world */
        if ((_column_idx + 20) <= _map_header.width) {
                fs_read(_map_fh, &_column[0], 20 * sizeof(struct world_column));
        }

        uint32_t column;
        for (column = 0; column < 20; column++) {
                uint32_t row;
                for (row = 0; row < 14; row++) {
                        uint16_t *page;
                        page = &COMPONENT(_layer, map).plane[0].page[0];

                        page[column + (32 * row)] = _column[column].cell[row].number;
                }
        }
}

void
component_world_mgr_on_draw(struct component *this __unused)
{
}

void
component_world_mgr_on_destroy(struct component *this __unused)
{
        fs_close(_map_fh);
}
