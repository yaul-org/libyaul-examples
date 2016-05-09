/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../blue.h"

void
component_world_mgr_on_init(struct component *this __unused)
{
        assert((THIS(world_mgr, world) >= 0) &&
               (THIS(world_mgr, world) < BLUE_WORLDS));

        THIS_P_DATA(world_mgr, coin_mgr) =
            (struct coin_mgr *)object_component_find(THIS(world_mgr, object),
                COMPONENT_ID_COIN_MGR);

        /* Open file */
        THIS_P_DATA(world_mgr, fh) = fs_open(blue_worlds[THIS(world_mgr, world)]);
        assert(THIS_P_DATA(world_mgr, fh) != NULL);

        /* Read header */
        THIS_P_DATA(world_mgr, map_header) = (struct blue_world_header *)malloc(
                sizeof(struct blue_world_header));
        assert(THIS_P_DATA(world_mgr, map_header) != NULL);

        fs_read(THIS_P_DATA(world_mgr, fh), THIS_P_DATA(world_mgr, map_header),
            sizeof(struct blue_world_header));

        fs_close(THIS_P_DATA(world_mgr, fh));
}

void
component_world_mgr_on_update(struct component *this __unused)
{
        (void)sprintf(text_buffer, "Hello from component world_mgr: \"%s\"\n",
            THIS_P_DATA(world_mgr, map_header)->name);
        cons_buffer(text_buffer);
}

void
component_world_mgr_on_draw(struct component *this __unused)
{
}

void
component_world_mgr_on_destroy(struct component *this __unused)
{
}
