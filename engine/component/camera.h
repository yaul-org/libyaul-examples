/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_CAMERA_H
#define ENGINE_CAMERA_H

#include "../engine.h"

struct camera {
        COMPONENT_DECLARATIONS

        uint16_t width;
        uint16_t height;
} camera;

extern void component_camera_on_init(struct component *);
extern void component_camera_on_update(struct component *);

#endif /* !ENGINE_CAMERA_H */
