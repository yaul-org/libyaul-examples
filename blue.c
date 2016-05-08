/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

struct blue_data blue_data;

#define BLUE_WORLD_INITIALIZER(_name, _prefix, _screens)                       \
        {                                                                      \
                .name = CC_STRINGIFY(_name),                                   \
                .map_filename = CC_STRINGIFY(CC_CONCAT(/WORLDS/, CC_CONCAT(_prefix,.MAP))), \
                .col_filename = CC_STRINGIFY(CC_CONCAT(/WORLDS/, CC_CONCAT(_prefix,.MAP))), \
                .obj_filename = CC_STRINGIFY(CC_CONCAT(/WORLDS/, CC_CONCAT(_prefix,.MAP))), \
                .screens = _screens                                            \
        }

const struct blue_world blue_worlds[] = {
        BLUE_WORLD_INITIALIZER("1-1 Blue", 11, 1)
};

void
main(void)
{
        engine_init();

        SCENE_ADD(SCENE_ID_SPLASH, splash, &blue_data);
        SCENE_ADD(SCENE_ID_TITLE, title, &blue_data);
        SCENE_ADD(SCENE_ID_GAME, game, &blue_data);

        scene_transition(SCENE_ID_GAME);

        engine_loop();
}
