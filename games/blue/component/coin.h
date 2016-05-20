/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_COIN_H
#define ENGINE_COIN_H

#include "../blue.h"

struct coin {
        COMPONENT_DECLARATIONS

        uint32_t ttl;
} __aligned (64);

extern void component_coin_on_init(struct component *);
extern void component_coin_on_update(struct component *);
extern void component_coin_on_draw(struct component *);
extern void component_coin_on_destroy(struct component *);

#endif /* !ENGINE_COIN_H */
