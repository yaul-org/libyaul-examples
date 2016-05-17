/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_BLUE_MGR_H
#define ENGINE_BLUE_MGR_H

#include "../blue.h"

struct blue_mgr {
        COMPONENT_DECLARATIONS

        fix16_vector2_t start_position;
} __aligned (64);

extern void component_blue_mgr_on_init(struct component *);
extern void component_blue_mgr_on_update(struct component *);
extern void component_blue_mgr_on_draw(struct component *);
extern void component_blue_mgr_on_destroy(struct component *);

#endif /* !ENGINE_BLUE_MGR_H */
