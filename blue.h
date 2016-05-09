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

struct blue_world_header;
struct blue_world_collider;
struct blue_world_map;

/* Objects */
#include "object/camera.h"
#include "object/world.h"
#include "object/blue.h"
#include "object/coin.h"

/* Non-engine components */
#include "component/coin_mgr.h"
#include "component/world_mgr.h"
#include "component/blue_mgr.h"

#define SCENE_ID_SPLASH 0
#define SCENE_ID_TITLE  1
#define SCENE_ID_GAME   2

#define OBJECT_ID_CAMERA        0x0001
#define OBJECT_ID_WORLD         0x0002
#define OBJECT_ID_BLUE          0x0003
#define OBJECT_ID_COIN          0x0004

#define COMPONENT_ID_COIN_MGR   0x4001
#define COMPONENT_ID_WORLD_MGR  0x4002
#define COMPONENT_ID_BLUE_MGR   0x4003

#define BLUE_WORLDS     1

struct blue_data {
};

struct blue_world_header {
        const char name[16];
        uint16_t width;
        uint16_t height;
        color_rgb555_t bg_color;
        fix16_t camera_speed;
        uint16_t start_delay;
        fix16_vector2_t blue_position;
        uint16_t collider_count;
} __packed;

struct blue_world_collider {
        uint16_t type;
        fix16_vector2_t position;
        int16_vector2_t cell_position;
        uint16_t width;
        uint16_t height;
} __packed;

struct blue_world_map {
        unsigned int coin:1;
        unsigned int cell_no:7;
} __packed;

extern struct blue_data blue_data;
extern const char *blue_worlds[];

#endif /* !BLUE_H */
