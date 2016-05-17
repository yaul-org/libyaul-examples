/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_CAMERA_MGR_H
#define ENGINE_CAMERA_MGR_H

#include "../blue.h"

struct camera_mgr {
        COMPONENT_DECLARATIONS

        fix16_t speed;
} __aligned (64);

extern void component_camera_mgr_on_init(struct component *);
extern void component_camera_mgr_on_update(struct component *);
extern void component_camera_mgr_on_draw(struct component *);
extern void component_camera_mgr_on_destroy(struct component *);

#endif /* !ENGINE_CAMERA_MGR_H */
