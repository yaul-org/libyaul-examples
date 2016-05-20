/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_COIN_MGR_H
#define ENGINE_COIN_MGR_H

#include "../blue.h"

struct coin_mgr {
        COMPONENT_DECLARATIONS

        uint32_t coins;
        void (*spawn)(struct component *, const fix16_vector2_t *);

        /* Private data */
        uint32_t _coin_cnt;
} __aligned (64);

extern void component_coin_mgr_on_init(struct component *);
extern void component_coin_mgr_on_update(struct component *);
extern void component_coin_mgr_on_draw(struct component *);
extern void component_coin_mgr_on_destroy(struct component *);

extern void component_coin_mgr_spawn(struct component *,
    const fix16_vector2_t *);

#endif /* !ENGINE_COIN_MGR_H */
