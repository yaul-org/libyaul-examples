/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_TRANSFORM_H
#define ENGINE_TRANSFORM_H

#include "../engine.h"

struct transform {
        COMPONENT_DECLARATIONS

        fix16_vector3_t position;
} __aligned (32);

extern void component_transform_on_init(struct component *);

#endif /* !ENGINE_TRANSFORM_H */
