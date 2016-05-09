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

        struct {
                struct coin_mgr *m_coin_mgr;
                void *m_fh;
                struct blue_world_header *m_map_header;
                struct blue_world_collider *m_map_colliders;
                struct blue_world_column m_column[20];
        } private_data;
} __aligned (64);

extern void component_world_mgr_on_init(struct component *);
extern void component_world_mgr_on_update(struct component *);
extern void component_world_mgr_on_draw(struct component *);
extern void component_world_mgr_on_destroy(struct component *);

#endif /* !ENGINE_WORLD_MGR_H */
