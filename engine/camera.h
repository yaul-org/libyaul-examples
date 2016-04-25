/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_CAMERA_H
#define ENGINE_CAMERA_H

#include <inttypes.h>
#include <math.h>

#include "component.h"

struct camera {
        COMPONENT_DECLARATIONS

        uint16_t width;
        uint16_t height;
} camera;

#endif /* !ENGINE_CAMERA_H */
