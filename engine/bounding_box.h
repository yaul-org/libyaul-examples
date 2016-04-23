/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_BOUNDING_BOX_H
#define ENGINE_BOUNDING_BOX_H

#include <math.h>

struct bounding_box {
        union {
                int16_vector2_t points[4];

                struct {
                        int16_vector2_t a;
                        int16_vector2_t b;
                        int16_vector2_t c;
                        int16_vector2_t d;
                } point;
        };
};

#endif /* !ENGINE_BOUNDING_BOX_H */
