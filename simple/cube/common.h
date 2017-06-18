/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef COMMON_H
#define COMMON_H

#define RGB888_TO_RGB555(r, g, b) (0x8000 | (((b) >> 3) << 10) |               \
    (((g) >> 3) << 5) | ((r) >> 3))

#define FIX16_VERTEX3(x, y, z)                                                 \
{                                                                              \
            {                                                                  \
                    F16((x)), F16((y)), F16((z)))                              \
            }                                                                  \
}

#define FIX16_VERTEX4(x, y, z, w)                                              \
{                                                                              \
            {                                                                  \
                    F16((x)), F16((y)), F16((z)), F16((w))                     \
            }                                                                  \
}

#endif /* !COMMON_H */
