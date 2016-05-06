/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_TRANSFORM_H
#define ENGINE_TRANSFORM_H

#include <yaul.h>

#include <inttypes.h>
#include <math.h>

#include "component.h"

struct transform {
        COMPONENT_DECLARATIONS

        fix16_vector3_t position;
} __aligned (32);

extern void component_transform_on_init(struct component *);

#endif /* !ENGINE_TRANSFORM_H */
