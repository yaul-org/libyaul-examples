/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef OBJECT_BLUE_H
#define OBJECT_BLUE_H

#include "blue.h"

#define OBJECT_ID_BLUE 2

struct object_blue {
        OBJECT_DECLARATIONS

        /* Public data */
        struct {
        } functions;

        const struct {
        } data;
};

extern struct object_blue object_blue;

#endif /* !OBJECT_BLUE_H */
