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

static struct transform *_transform;
static struct camera *_camera;
static struct camera_mgr *_camera_mgr;
static struct coin_mgr *_coin_mgr;
static struct layer *_layer;
static void *_map_fh;
static struct world_header _map_header;
static struct world_collider _map_colliders[COLLIDERS_MAX];
static struct world_coin _map_coins[COINS_MAX];
static bool _init = true;
static uint32_t _column_start_idx = 0;
static uint32_t _column_end_idx = 0;
static uint32_t _plane_idx = 0;
static uint32_t _page_column_idx = 0;
static fix16_t _last_scroll_x = F16(0.0f);

static uint8_t _character_pattern_base[2048];

static void _map_column_update(uint32_t);

void
component_world_mgr_on_init(struct component *this __unused)
{
        assert((THIS(world_mgr, world) >= 0) &&
               (THIS(world_mgr, world) < BLUE_WORLDS));

        memb_init(&_collider_pool);

        _transform = (struct transform *)object_component_find(
                THIS(world_mgr, object), COMPONENT_ID_TRANSFORM);
        assert(_transform != NULL);

        _camera = (struct camera *)objects_component_find(COMPONENT_ID_CAMERA);
        assert(_camera != NULL);

        _camera_mgr = (struct camera_mgr *)objects_component_find(
                COMPONENT_ID_CAMERA_MGR);
        assert(_camera_mgr != NULL);

        _coin_mgr = (struct coin_mgr *)object_component_find(
                THIS(world_mgr, object), COMPONENT_ID_COIN_MGR);
        assert(_coin_mgr != NULL);

        _layer = (struct layer *)object_component_find(
                THIS(world_mgr, object), COMPONENT_ID_LAYER);
        assert(_layer != NULL);

        /* Open file */
        const char *world_filename;
        world_filename = blue_worlds[THIS(world_mgr, world)];

        _map_fh = fs_open(world_filename);
        assert(_map_fh != NULL);

        /* Read header */
        fs_read(_map_fh, &_map_header, sizeof(struct world_header), 1);

        assert((_map_header.collider_count > 0) &&
               (_map_header.collider_count <= COLLIDERS_MAX));
        assert((_map_header.coin_count > 0) &&
               (_map_header.coin_count <= COINS_MAX));

        /* Read in collision data */
        fs_read(_map_fh, &_map_colliders[0], sizeof(struct world_collider),
            _map_header.collider_count);

        /* Read in coin data */
        fs_read(_map_fh, &_map_coins[0], sizeof(struct world_coin),
            _map_header.coin_count);

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

        /* Set the camera speed and start delay */
        COMPONENT(_camera_mgr, speed) = _map_header.camera_speed;
}

void
component_world_mgr_on_update(struct component *this __unused)
{
        (void)sprintf(text_buffer, "Hello from component world_mgr\n\"%s\"\n",
            _map_header.name);
        cons_buffer(text_buffer);

        /* Determine how much the camera has traveled from the world's
         * object origion. */
        struct transform *camera_transform;
        camera_transform = (struct transform *)object_component_find(
                COMPONENT(_camera, object), COMPONENT_ID_TRANSFORM);
        assert(camera_transform != NULL);

        (void)sprintf(text_buffer, "\n\nwidth=%i\ncolumn_idx=%i\ncolumn_idx2=%i\n_plane_idx=%i\n",
            (int)_map_header.width,
            (int)_column_end_idx,
            (int)_column_start_idx,
            (int)_plane_idx);
        cons_buffer(text_buffer);

        fix16_t scroll_x;
        scroll_x = fix16_sub(COMPONENT(camera_transform, position).x, COMPONENT(_transform, position).x);
        fix16_to_str(scroll_x, text_buffer, 7);
        cons_buffer("scroll_x=");
        cons_buffer(text_buffer);
        cons_buffer("\n");

        fix16_to_str(fix16_mod(scroll_x, F16(16.0f)), text_buffer, 7);
        cons_buffer("scroll_x%16=");
        cons_buffer(text_buffer);
        cons_buffer("\n");

        /* Has the camera moved since the last frame? */
        bool moved;
        moved = fix16_sub(scroll_x, _last_scroll_x) != F16(0.0f);

        /* Have we advanced to see a new column? */
        bool new_column;
        new_column = fix16_integral(fix16_mod(scroll_x, F16(16.0f))) == 0;

        /* Are we at the end of the world? */
        bool world_end;
        world_end = _column_end_idx >= _map_header.width;

        if (!_init && (!moved || !new_column || world_end)) {
                cons_buffer("State3\n");
                return;
        }

        if ((_column_start_idx % 32) == 0) {
                cons_buffer("State1\n");

                _init = false;

                _plane_idx = 0;
                _page_column_idx = 0;

                /* Update map */
                _map_column_update(20 + 1);
        } else {
                /* Update at the end of the current plane's page. */
                cons_buffer("State2\n");

                /* Update map */
                _map_column_update(1);
        }

        _column_start_idx++;

        if ((_column_start_idx % 32) == 0) {
                _column_end_idx = _column_start_idx;
        }

        _last_scroll_x = scroll_x;
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

static void
_map_column_update(uint32_t columns)
{
        assert((columns >= 1) && (columns <= (20 + 1)));

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
            (_column_end_idx * sizeof(struct world_column));
        fs_seek(_map_fh, file_offset, SEEK_SET);

        /* When reaching the end of the first page (32x32) of plane A,
         * move onto the next page in plane B. */
        if ((_column_end_idx > 0) && ((_column_end_idx % 32) == 0)) {
                _plane_idx = 1;
                _page_column_idx = 0;
        }

        uint16_t *page;
        page = &COMPONENT(_layer, map).plane[_plane_idx].page[0];

        struct world_column map_columns[20 + 1];
        fs_read(_map_fh, &map_columns[0], sizeof(struct world_column), columns);

        /* Update map */
        uint32_t column;
        for (column = 0; column < columns; column++) {
                uint32_t row;
                for (row = 0; row < 14; row++) {
                        uint16_t cell_number;
                        cell_number = map_columns[column].cell[row].number;

                        uint32_t page_idx;
                        page_idx = _page_column_idx + column + (32 * row);

                        page[page_idx] = cell_number;
                }

                _page_column_idx++;

                _column_end_idx++;
        }
}
