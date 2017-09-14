/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef SCENE_H
#define SCENE_H

#include "engine.h"

#define SCENE_ADD(scene_id, scene, data) do {                                  \
        extern void __CONCAT(scene_, __CONCAT(scene, _init))(struct scene_ctx *); \
        extern void __CONCAT(scene_, __CONCAT(scene, _update))(struct scene_ctx *); \
        extern void __CONCAT(scene_, __CONCAT(scene, _draw))(struct scene_ctx *); \
        extern void __CONCAT(scene_, __CONCAT(scene, _exit))(struct scene_ctx *); \
                                                                               \
        scene_add(__STRING(scene),                                             \
            scene_id,                                                          \
            __CONCAT(scene_, __CONCAT(scene, _init)),                          \
            __CONCAT(scene_, __CONCAT(scene, _update)),                        \
            __CONCAT(scene_, __CONCAT(scene, _draw)),                          \
            __CONCAT(scene_, __CONCAT(scene, _exit)),                          \
            data);                                                             \
} while(false)

struct scene_ctx {
        uint32_t sc_frames;
        void *sc_data;
};

extern uint32_t scene_current(void);
extern void scene_add(const char *, uint32_t, void (*)(struct scene_ctx *),
    void (*)(struct scene_ctx *), void (*)(struct scene_ctx *),
    void (*)(struct scene_ctx *), void *);
extern void scene_handler_draw(void);
extern void scene_handler_update(void);
extern void scene_init(void);
extern void scene_transition(uint32_t);

#endif /* !SCENE_H */
