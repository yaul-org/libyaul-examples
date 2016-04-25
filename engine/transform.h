/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_TRANSFORM_H
#define ENGINE_TRANSFORM_H

#include <inttypes.h>
#include <math.h>

#include "component.h"

struct transform {
        COMPONENT_DECLARATIONS

        fix16_vector3_t position;
        const int16_vector2_t screen[4]; /* Do not modify directly */
} transform;

#endif /* !ENGINE_TRANSFORM_H */
