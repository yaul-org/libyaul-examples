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

#include "engine.h"

#include "blue_palette.h"

/* Objects */
#include "object_camera.h"
#include "object_world.h"
#include "object_blue.h"
#include "object_coin.h"

/* Non-engine components */
#include "component_coin_mgr.h"

#define SCENE_ID_SPLASH 0
#define SCENE_ID_TITLE  1
#define SCENE_ID_GAME   2

#define OBJECT_ID_CAMERA                0x0001
#define OBJECT_ID_WORLD                 0x0002
#define OBJECT_ID_BLUE                  0x0003
#define OBJECT_ID_COIN                  0x0004

#define COMPONENT_ID_COIN_MGR           0x8000

struct blue_data {
};

extern struct blue_data blue_data;

#endif /* !BLUE_H */
