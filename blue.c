/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

struct blue_data blue_data;

#define BLUE_WORLD_INITIALIZER(prefix)                                        \
        {                                                                      \
                .map_filename = CC_STRINGIFY(CC_CONCAT(/WORLDS/, CC_CONCAT(prefix,.MAP)))
        }

const struct blue_world blue_worlds[] = {
        BLUE_WORLD_INITIALIZER(11)
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
