/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

struct blue_data blue_data;

const char *blue_worlds[] = {
        "/WORLDS/1_1.MAP"
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
