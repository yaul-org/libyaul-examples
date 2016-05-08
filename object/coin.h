/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef OBJECT_COIN_H
#define OBJECT_COIN_H

#include "../blue.h"

struct object_coin {
        OBJECT_DECLARATIONS

        int16_t value;
} __aligned (64);

extern const struct object_coin object_coin;

#endif /* !OBJECT_COIN_H */
