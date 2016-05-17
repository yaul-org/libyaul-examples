/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

#define COIN_SPAWN_COUNT_MAX    256
#define COIN_VALUE              100

static struct object object_coins[COIN_SPAWN_COUNT_MAX];

void
component_coin_mgr_on_init(struct component *this)
{
        assert(this != NULL);
        assert((THIS(coin_mgr, coins) >= 1) &&
               (THIS(coin_mgr, coins) <= COIN_SPAWN_COUNT_MAX));
        assert(this != NULL);

        THIS_P_DATA(coin_mgr, coin_cnt) = 0;

        uint32_t coin_idx;
        for (coin_idx = 0; coin_idx < COIN_SPAWN_COUNT_MAX; coin_idx++) {
                struct object *inst_object_coin;
                inst_object_coin = &object_coins[coin_idx];

                object_instantiate((const struct object *)&object_coin,
                    (struct object *)inst_object_coin, sizeof(object_coin));

                objects_object_child_add(
                        (struct object *)THIS(coin_mgr, object),
                        inst_object_coin);
        }
}

void
component_coin_mgr_on_update(struct component *this __unused)
{
}

void
component_coin_mgr_on_draw(struct component *this __unused)
{
}

void
component_coin_mgr_on_destroy(struct component *this __unused)
{
        uint32_t coin_idx;
        for (coin_idx = 0; coin_idx < COIN_SPAWN_COUNT_MAX; coin_idx++) {
                struct object *object_coin;
                object_coin = &object_coins[coin_idx];

                struct sprite *sprite;
                sprite = (struct sprite *)object_component_find(object_coin,
                    COMPONENT_ID_SPRITE);
                struct coin *coin;
                coin = (struct coin *)object_component_find(object_coin,
                    COMPONENT_ID_COIN);

                OBJECT(object_coin, active) = false;
                COMPONENT(sprite, visible) = false;
                COMPONENT(coin, ttl) = 0;

                objects_object_remove(object_coin);
                object_destroy(object_coin);
        }
}

void
component_coin_mgr_spawn(struct component *this __unused,
    const fix16_vector2_t *spawn_pos)
{
        struct object *object_coin;
        object_coin = NULL;

        struct sprite *sprite;
        sprite = NULL;
        struct coin *coin;
        coin = NULL;

        /* Look for an inactive object coin */
        uint32_t coin_idx;
        for (coin_idx = THIS_P_DATA(coin_mgr, coin_cnt);
             coin_idx < COIN_SPAWN_COUNT_MAX;
             coin_idx++) {
                object_coin = &object_coins[coin_idx];

                sprite = (struct sprite *)object_component_find(object_coin,
                    COMPONENT_ID_SPRITE);
                coin = (struct coin *)object_component_find(object_coin,
                    COMPONENT_ID_COIN);

                /* Spawn if either not active or not visible and TTL > 0 */
                if (!OBJECT(object_coin, active) ||
                    (!COMPONENT(sprite, visible) && (COMPONENT(coin, ttl) > 0))) {
                        break;
                }
        }

        /* Make sure we can still actually spawn */
        assert(object_coin != NULL);

        OBJECT(object_coin, active) = true;

        struct transform *transform;
        transform = (struct transform *)object_component_find(object_coin,
            COMPONENT_ID_TRANSFORM);

        COMPONENT(transform, position).x = spawn_pos->x;
        COMPONENT(transform, position).y = spawn_pos->y;

        COMPONENT(sprite, visible) = false;
        COMPONENT(coin, ttl) = 0;

        THIS_P_DATA(coin_mgr, coin_cnt)++;
        THIS_P_DATA(coin_mgr, coin_cnt) &= (COIN_SPAWN_COUNT_MAX - 1);
}
