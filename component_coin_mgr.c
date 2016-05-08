/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "blue.h"

#define COIN_COUNT_MAX 64

static struct object_coin object_coins[COIN_COUNT_MAX];

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
                struct object_coin *inst_object_coin;
                inst_object_coin = &object_coins[coin_idx];

                object_instantiate((const struct object *)&object_coin,
                    (struct object *)inst_object_coin, sizeof(object_coin));

                objects_object_child_add(
                        (struct object *)THIS(coin_mgr, object),
                        (struct object *)inst_object_coin);
        }
}

void
component_coin_mgr_on_update(struct component *this __unused)
{
        cons_buffer("Hello from component coin_mgr\n");

        /* Free all coins no longer active or visible */
        uint32_t coin_idx;
        for (coin_idx = 0; coin_idx < COIN_COUNT_MAX; coin_idx++) {
                struct object_coin *object_coin;
                object_coin = &object_coins[coin_idx];

                if (!OBJECT(object_coin, active)) {
                        continue;
                }

                continue;

                struct transform *transform;
                transform = (struct transform *)OBJECT_COMPONENT(object_coin,
                    COMPONENT_ID_TRANSFORM);

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
        uint32_t coin_idx;
        for (coin_idx = 0; coin_idx < COIN_COUNT_MAX; coin_idx++) {
                struct object_coin *object_coin;
                object_coin = &object_coins[coin_idx];

                OBJECT(object_coin, active) = false;
        }
}

void
component_coin_mgr_spawn(struct component *this __unused, fix16_t spawn_x,
    fix16_t spawn_y, int16_t value)
{
        struct object_coin *object_coin;
        object_coin = NULL;

        /* Look for an inactive object coin */
        uint32_t coin_idx;
        for (coin_idx = THIS_P_DATA(coin_mgr, coin); coin_idx < COIN_COUNT_MAX;
             coin_idx++) {
                object_coin = &object_coins[coin_idx];

                /* Skip over active object coins */
                if (!OBJECT(object_coin, active)) {
                        break;
                }
        }

        /* Make sure we can still actually spawn */
        assert(object_coin != NULL);

        struct transform *transform;
        transform = (struct transform *)object_component_find(
                (struct object *)object_coin, COMPONENT_ID_TRANSFORM);

        COMPONENT(transform, position).x = spawn_x;
        COMPONENT(transform, position).y = spawn_y;

        OBJECT(object_coin, active) = true;
        OBJECT(object_coin, value) = value;

        THIS_P_DATA(coin_mgr, coin)++;
        THIS_P_DATA(coin_mgr, coin) &= (COIN_COUNT_MAX - 1);
}
