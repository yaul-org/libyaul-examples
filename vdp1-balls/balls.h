/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef BALL_H
#define BALL_H

#include <yaul.h>

#include "q0_12_4.h"

struct ball {
        q0_12_4_t speed; /* 2-bytes */
        q0_12_4_t pos_x; /* 2-bytes */
        q0_12_4_t pos_y; /* 2-bytes */
};

struct balls_config {
        struct ball *balls;
        uint16_t count;

        void *sprite_tex_base;
        void *sprite_pal_base;

        uint8_t dma_tag;
};

struct balls_handle;

typedef struct balls_handle *balls_handle_t;

typedef void (*balls_load_callback)(balls_handle_t handle);

balls_handle_t balls_init(const struct balls_config *config);
void balls_sprite_load(balls_handle_t handle, balls_load_callback callback);
void balls_position_update(balls_handle_t handle, const uint16_t count);
void balls_position_clamp(balls_handle_t handle, const uint16_t count);

void balls_cmdt_list_init(balls_handle_t handle, vdp1_cmdt_t *cmdts,
    const uint16_t count);
void balls_cmdt_list_update(balls_handle_t handle, vdp1_cmdt_t *cmdts,
    const uint16_t count);

#endif /* !BALL_H */
