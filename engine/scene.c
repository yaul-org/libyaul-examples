/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "scene.h"

#define STATES_MAX 16

struct state;

TAILQ_HEAD(states, state);

struct state {
        struct scene_ctx scene_ctx;

        const char *state_name;

        uint32_t state_id;

        void (*state_init)(struct scene_ctx *);
        void (*state_update)(struct scene_ctx *);
        void (*state_draw)(struct scene_ctx *);
        void (*state_exit)(struct scene_ctx *);

        TAILQ_ENTRY(state) entries;
};

static struct {
        bool initialized;

        struct states states;

        struct state *prev_state;
        struct state *cur_state;
} _state;

void
scene_init(void)
{
        if (_state.initialized) {
                return;
        }

        TAILQ_INIT(&_state.states);

        _state.prev_state = NULL;
        _state.cur_state = NULL;

        _state.initialized = true;
}

void
scene_add(const char *state_name, uint32_t state_id,
    void (*state_init)(struct scene_ctx *),
    void (*state_update)(struct scene_ctx *),
    void (*state_draw)(struct scene_ctx *),
    void (*state_exit)(struct scene_ctx *),
    void *data)
{
        assert(_state.initialized);

        struct state *state;
        state = (struct state *)malloc(sizeof(struct state));
        assert(state != NULL);

        state->state_name = state_name;
        state->state_id = state_id;
        state->state_init = state_init;
        state->state_update = state_update;
        state->state_draw = state_draw;
        state->state_exit = state_exit;

        /* Initialize state context */
        state->scene_ctx.sc_frames = 0;
        state->scene_ctx.sc_data = data;

        TAILQ_INSERT_TAIL(&_state.states, state, entries);
}

void
scene_transition(uint32_t state_id)
{
        assert(_state.initialized);

        _state.prev_state = _state.cur_state;

        struct state *prev_state;
        prev_state = _state.prev_state;
        if (prev_state != NULL) {
                if (prev_state->state_exit != NULL) {
                        prev_state->state_exit(&prev_state->scene_ctx);
                }

                prev_state->scene_ctx.sc_frames = 0;
        }

        /* Check if transitioning state is valid */
        struct state *state;

        bool state_found;
        state_found = false;
        TAILQ_FOREACH (state, &_state.states, entries) {
                if (state->state_id == state_id) {
                        state_found = true;
                        break;
                }
        }
        assert(state_found);
        _state.cur_state = state;

        struct state *cur_state;
        cur_state = _state.cur_state;

        if (cur_state->state_init != NULL) {
                cur_state->state_init(&cur_state->scene_ctx);
        }
}

void
scene_handler_update(void)
{
        assert(_state.initialized);

        struct state *state;
        state = _state.cur_state;

        state->scene_ctx.sc_frames++;

        if (state->state_update != NULL) {
                state->state_update(&state->scene_ctx);
        }
}

void
scene_handler_draw(void)
{
        assert(_state.initialized);

        struct state *state;
        state = _state.cur_state;

        if (state->scene_ctx.sc_frames == 0) {
                return;
        }

        if (state->state_draw != NULL) {
                state->state_draw(&state->scene_ctx);
        }
}
