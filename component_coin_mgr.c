/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "component_coin_mgr.h"

#define COIN_COUNT_MAX 64

static struct object_coin object_coins[COIN_COUNT_MAX];
static struct sprite sprites[COIN_COUNT_MAX];
static struct transform transforms[COIN_COUNT_MAX];
static struct collider colliders[COIN_COUNT_MAX];

void
component_coin_mgr_on_init(struct component *this)
{
        assert(this != NULL);
        assert((THIS(coin_mgr, coins) >= 1) &&
               (THIS(coin_mgr, coins) <= COIN_COUNT_MAX));
        assert(this != NULL);

        THIS_P_DATA(coin_mgr, coin) = 0;

        uint32_t coin_idx;
        for (coin_idx = 0; coin_idx < COIN_COUNT_MAX; coin_idx++) {
                struct object_coin *object_coin;
                object_coin = &object_coins[coin_idx];

                struct sprite *sprite;
                sprite = &sprites[coin_idx];

                struct transform *transform;
                transform = &transforms[coin_idx];

                struct collider *collider;
                collider = &colliders[coin_idx];

                COMPONENT(transform, active) = true;
                COMPONENT(transform, id) = COMPONENT_ID_TRANSFORM;
                COMPONENT(transform, object) = (struct object *)object_coin;
                COMPONENT(transform, position).x = F16(0.0f);
                COMPONENT(transform, position).y = F16(0.0f);
                COMPONENT(transform, position).z = F16(1.0f);

                COMPONENT(collider, active) = true;
                COMPONENT(collider, id) = COMPONENT_ID_COLLIDER;
                COMPONENT(collider, object) = (const struct object *)object_coin;
                COMPONENT(collider, width) = 8;
                COMPONENT(collider, height) = 8;
                COMPONENT(collider, trigger) = false;
                COMPONENT(collider, fixed) = false;
                COMPONENT(collider, show) = true;
                COMPONENT(collider, on_init) = &component_collider_on_init;
                COMPONENT(collider, on_update) = NULL;
                COMPONENT(collider, on_draw) = NULL;
                COMPONENT(collider, on_destroy) = NULL;

                COMPONENT(sprite, active) = true;
                COMPONENT(sprite, id) = COMPONENT_ID_SPRITE;
                COMPONENT(sprite, object) = (const struct object *)object_coin;
                COMPONENT(sprite, width) = 8;
                COMPONENT(sprite, height) = 8;
                COMPONENT(sprite, on_init) = &component_sprite_on_init;
                COMPONENT(sprite, on_update) = &component_sprite_on_update;
                COMPONENT(sprite, on_draw) = component_sprite_on_draw;
                COMPONENT(sprite, on_destroy) = NULL;

                OBJECT(object_coin, active) = false;
                OBJECT(object_coin, id) = OBJECT_ID_COIN;
                OBJECT_COMPONENT(object_coin, 0) = (struct component *)transform;
                OBJECT_COMPONENT(object_coin, 1) = (struct component *)sprite;
                OBJECT_COMPONENT(object_coin, 2) = (struct component *)collider;
                OBJECT(object_coin, component_count) = 3;
                OBJECT(object_coin, value) = 0;

                object_init((struct object *)object_coin);
                objects_object_child_add((struct object *)THIS(coin_mgr, object),
                    (struct object *)object_coin);
        }
}

void
component_coin_mgr_on_update(struct component *this __unused)
{
        uint32_t coin_idx;
        for (coin_idx = 0; coin_idx < COIN_COUNT_MAX; coin_idx++) {
                struct object_coin *object_coin;
                object_coin = &object_coins[coin_idx];

                if (!OBJECT(object_coin, active)) {
                        continue;
                }

                continue;

                struct transform *sprite __unused;
                sprite = &sprite[coin_idx];

                struct transform *transform;
                transform = &transforms[coin_idx];

                COMPONENT(transform, position).x = F16(0.0f);
                COMPONENT(transform, position).y = F16(0.0f);

                OBJECT(object_coin, active) = false;
                OBJECT(object_coin, value) = 0;
        }
}

void
component_coin_mgr_on_draw(struct component *this __unused)
{
}

void
component_coin_mgr_on_destroy(struct component *this __unused)
{
}

void
component_coin_mgr_spawn(struct component *this __unused, fix16_t spawn_x,
    fix16_t spawn_y, int16_t value)
{
        uint32_t coin_idx;
        for (coin_idx = THIS_P_DATA(coin_mgr, coin);
             coin_idx < COIN_COUNT_MAX;
             coin_idx++) {
                struct object_coin *object_coin;
                object_coin = &object_coins[coin_idx];

                /* Skip over active object coins */
                if (!OBJECT(object_coin, active)) {
                        continue;
                }

                struct transform *transform;
                transform = &transforms[coin_idx];

                COMPONENT(transform, position).x = spawn_x;
                COMPONENT(transform, position).y = spawn_y;

                OBJECT(object_coin, active) = true;
                OBJECT(object_coin, value) = value;
        }

        THIS_P_DATA(coin_mgr, coin) &= (COIN_COUNT_MAX - 1);
}
