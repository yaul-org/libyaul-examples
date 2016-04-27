/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef SCENE_H
#define SCENE_H

#include <assert.h>
#include <inttypes.h>

#include <sys/queue.h>

#define SCENE_ADD(scene_id, scene, data) do {                                  \
        extern void CC_CONCAT(scene_, CC_CONCAT(scene, _init))(struct scene_ctx *); \
        extern void CC_CONCAT(scene_, CC_CONCAT(scene, _update))(struct scene_ctx *); \
        extern void CC_CONCAT(scene_, CC_CONCAT(scene, _draw))(struct scene_ctx *); \
        extern void CC_CONCAT(scene_, CC_CONCAT(scene, _exit))(struct scene_ctx *); \
                                                                               \
        scene_add(CC_STRINGIFY(scene),                                         \
            scene_id,                                                          \
            CC_CONCAT(scene_, CC_CONCAT(scene, _init)),                        \
            CC_CONCAT(scene_, CC_CONCAT(scene, _update)),                      \
            CC_CONCAT(scene_, CC_CONCAT(scene, _draw)),                        \
            CC_CONCAT(scene_, CC_CONCAT(scene, _exit)),                        \
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
