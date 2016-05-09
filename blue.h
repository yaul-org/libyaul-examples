/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef BLUE_H
#define BLUE_H

#include <yaul.h>
#include <tga.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine/engine.h"

#include "palette.h"

/* Objects */
#include "object/camera.h"
#include "object/world.h"
#include "object/blue.h"
#include "object/coin.h"

/* Non-engine components */
#include "component/coin_mgr.h"

#define SCENE_ID_SPLASH 0
#define SCENE_ID_TITLE  1
#define SCENE_ID_GAME   2

#define OBJECT_ID_CAMERA                0x0001
#define OBJECT_ID_WORLD                 0x0002
#define OBJECT_ID_BLUE                  0x0003
#define OBJECT_ID_COIN                  0x0004

#define COMPONENT_ID_COIN_MGR           0x0001

struct blue_data {
};

struct blue_world {
        const char *map_filename;
};

struct blue_world_format {
        const char *name;
        uint16_t width;
        uint16_t height
        color_rgb555_t bg_color;
        fix16_t camera_speed;
        uint16_t start_delay;
        fix16_vector2_t blue_position;
        uint16_t collider_count;
        struct {
                uint16_t type;
                fix16_vector2_t position;
                int16_vector2_t cell_position;
                uint16_t width;
                uint16_t height;
        } __packed *collders;
} __packed;

extern struct blue_data blue_data;
extern const struct blue_world blue_worlds[];

#endif /* !BLUE_H */
