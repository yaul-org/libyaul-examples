/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

#define COLLIDERS_MAX 16

struct world_header {
        const char name[16];
        uint16_t width;
        uint16_t height;
        color_rgb555_t bg_color;
        fix16_t camera_speed;
        uint16_t start_delay;
        fix16_t blue_position_x;
        fix16_t blue_position_y;
        uint16_t collider_count;
} __packed;

struct world_collider {
        uint16_t type;
        fix16_vector2_t position;
        int16_vector2_t cell_position;
        uint16_t width;
        uint16_t height;
} __packed;

struct world_column {
        struct {
                unsigned int :2;
                unsigned int trigger:1;
                unsigned int collider:1;
                unsigned int coin:1;
                unsigned int cell_no:3;
        } __packed cell[14]; /* SCREEN_WIDTH / CELL_HEIGHT */
} __packed;

MEMB(_collider_pool, struct collider, COLLIDERS_MAX, sizeof(struct collider));

static uint32_t _singleton = 0;
static struct coin_mgr *_coin_mgr;
static void *_map_fh;
static struct world_header _map_header;
static struct world_collider _map_colliders[COLLIDERS_MAX] __unused;
static struct world_column _column[20] __unused;

void
component_world_mgr_on_init(struct component *this __unused)
{
        assert((THIS(world_mgr, world) >= 0) &&
               (THIS(world_mgr, world) < BLUE_WORLDS));
        /* Singleton component */
        assert(_singleton == 0);

        memb_init(&_collider_pool);

        _coin_mgr = (struct coin_mgr *)object_component_find(
                THIS(world_mgr, object), COMPONENT_ID_COIN_MGR);

        /* Open file */
        const char *world_filename;
        world_filename = blue_worlds[THIS(world_mgr, world)];

        _map_fh = fs_open(world_filename);
        assert(_map_fh != NULL);

        /* Read header */
        fs_read(_map_fh, &_map_header, sizeof(struct world_header));

        _singleton++;
}

void
component_world_mgr_on_update(struct component *this __unused)
{
        /* Calculate offset to actual map data */
        uint32_t file_offset;
        file_offset =
            /* Header */
            sizeof(struct world_header) +
            /* Colliders */
            (_map_header.collider_count * sizeof(struct world_collider)) +
            /* Column */
            (0 * sizeof(struct world_column));
        fs_seek(_map_fh, file_offset, SEEK_SET);

        struct world_column column __unused;
        fs_read(_map_fh, &column, sizeof(struct world_column));

        (void)sprintf(text_buffer, "Hello from component world_mgr:\n\"%s\"\n",
            _map_header.name);
        cons_buffer(text_buffer);
}

void
component_world_mgr_on_draw(struct component *this __unused)
{
}

void
component_world_mgr_on_destroy(struct component *this __unused)
{
}
