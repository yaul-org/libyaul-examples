/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef VDP1_BALLS_H
#define VDP1_BALLS_H

#include <yaul.h>

#include <sys/cdefs.h>

#define RESOLUTION_WIDTH    (352)
#define RESOLUTION_HEIGHT   (224)

#define SCREEN_WIDTH    (352)
#define SCREEN_HEIGHT   (224)

#define SCREEN_HWIDTH_Q         Q0_12_4((float)SCREEN_WIDTH / 2.0f)
#define SCREEN_HHEIGHT_Q        Q0_12_4((float)SCREEN_HEIGHT / 2.0f)

#define BALL_MAX_COUNT  (1024)

extern void *_romdisk;

#endif /* !VDP1_BALLS_H */
