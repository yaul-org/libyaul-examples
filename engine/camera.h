/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_CAMERA_H
#define ENGINE_CAMERA_H

#include <yaul.h>

#include <math.h>
#include <inttypes.h>

#include "component.h"

struct camera {
        COMPONENT_DECLARATIONS

        uint16_t width;
        uint16_t height;
} camera;

extern void component_camera_init(struct component *);

#endif /* !ENGINE_CAMERA_H */
