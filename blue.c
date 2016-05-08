/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

struct blue_data blue_data;

const color_rgb555_t blue_palette[] = {
        COLOR_RGB555_INITIALIZER(0x1F, 0x1F, 0x1F),
        COLOR_RGB555_INITIALIZER(0x04, 0x14, 0x1A),
        COLOR_RGB555_INITIALIZER(0x1F, 0x0F, 0x09),
        COLOR_RGB555_INITIALIZER(0x13, 0x19, 0x1F),
        COLOR_RGB555_INITIALIZER(0x19, 0x18, 0x1F),
        COLOR_RGB555_INITIALIZER(0x1F, 0x1F, 0x14),
        COLOR_RGB555_INITIALIZER(0x10, 0x10, 0x10),
        COLOR_RGB555_INITIALIZER(0x18, 0x18, 0x18),
        COLOR_RGB555_INITIALIZER(0x04, 0x10, 0x19),
        COLOR_RGB555_INITIALIZER(0x1F, 0x07, 0x02),
        COLOR_RGB555_INITIALIZER(0x07, 0x13, 0x1B),
        COLOR_RGB555_INITIALIZER(0x14, 0x13, 0x1F),
        COLOR_RGB555_INITIALIZER(0x1F, 0x19, 0x04),
        COLOR_RGB555_INITIALIZER(0x00, 0x00, 0x00),
        COLOR_RGB555_INITIALIZER(0x10, 0x15, 0x1F),
        COLOR_RGB555_INITIALIZER(0x1F, 0x1F, 0x08),
        COLOR_RGB555_INITIALIZER(0x1F, 0x04, 0x04),
        COLOR_RGB555_INITIALIZER(0x1C, 0x10, 0x08),
        COLOR_RGB555_INITIALIZER(0x1E, 0x18, 0x08),
        COLOR_RGB555_INITIALIZER(0x08, 0x10, 0x18),
        COLOR_RGB555_INITIALIZER(0x08, 0x18, 0x1E),
        COLOR_RGB555_INITIALIZER(0x10, 0x1F, 0x10),
        COLOR_RGB555_INITIALIZER(0x18, 0x10, 0x10),
        COLOR_RGB555_INITIALIZER(0x10, 0x10, 0x1F),
        COLOR_RGB555_INITIALIZER(0x00, 0x14, 0x08),
        COLOR_RGB555_INITIALIZER(0x14, 0x0C, 0x1C),
        COLOR_RGB555_INITIALIZER(0x18, 0x10, 0x1F)
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
