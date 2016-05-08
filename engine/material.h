/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_MATERIAL_H
#define ENGINE_MATERIAL_H

#include "engine.h"

struct material {
        bool pseudo_trans;
        color_rgb555_t solid_color;
};

#endif /* !ENGINE_MATERIAL_H */
