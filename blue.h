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

/* Non-engine components */
#include "component/coin_mgr.h"
#include "component/world_mgr.h"
#include "component/blue_mgr.h"
#include "component/coin.h"
#include "component/camera_mgr.h"

#define SCENE_ID_SPLASH 0
#define SCENE_ID_TITLE  1
#define SCENE_ID_GAME   2

#define OBJECT_ID_CAMERA        0x0001
#define OBJECT_ID_WORLD         0x0002
#define OBJECT_ID_BLUE          0x0003
#define OBJECT_ID_COIN          0x0004

#define COMPONENT_ID_COIN_MGR   0x1001
#define COMPONENT_ID_WORLD_MGR  0x1002
#define COMPONENT_ID_BLUE_MGR   0x1003
#define COMPONENT_ID_COIN       0x1004
#define COMPONENT_ID_CAMERA_MGR 0x1005

#define BLUE_WORLDS     1

struct blue_data {
};

/* Objects */
extern struct object object_camera;
extern struct object object_world;
extern struct object object_blue;
extern const struct object object_coin;

extern struct blue_data blue_data;
extern const char *blue_worlds[];

#endif /* !BLUE_H */
