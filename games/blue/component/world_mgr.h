/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_WORLD_MGR_H
#define ENGINE_WORLD_MGR_H

#include "../blue.h"

struct world_mgr {
        COMPONENT_DECLARATIONS

        int32_t world;

        struct {
        } functions;
} __aligned (64);

extern void component_world_mgr_on_init(struct component *);
extern void component_world_mgr_on_update(struct component *);
extern void component_world_mgr_on_draw(struct component *);
extern void component_world_mgr_on_destroy(struct component *);

#endif /* !ENGINE_WORLD_MGR_H */
