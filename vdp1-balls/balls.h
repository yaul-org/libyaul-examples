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

typedef struct balls {
        q0_12_4_t *pos_x;
        q0_12_4_t *pos_y;
        int16_t *cmd_xa;
        int16_t *cmd_ya;
} balls_t;

typedef struct balls_config {
        const balls_t *balls;
        uint16_t count;
        q0_12_4_t speed;
} balls_config_t;

struct balls_handle;

typedef struct balls_handle balls_handle_t;

typedef void (*balls_load_callback_t)(balls_handle_t *handle);

balls_handle_t *balls_init(const balls_config_t config);

void balls_assets_init(balls_handle_t *handle);
void balls_assets_load(balls_handle_t *handle);

void balls_position_update(balls_handle_t *handle, uint16_t count);
void balls_position_clamp(balls_handle_t *handle, uint16_t count);

void balls_cmdts_update(balls_handle_t *handle, uint16_t count);

void balls_cmdts_put(balls_handle_t *handle, uint16_t index, uint16_t count);
void balls_cmdts_position_put(balls_handle_t *handle, uint16_t index, uint16_t count);

#endif /* !BALL_H */
