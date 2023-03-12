/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#include <fiber.h>

static fiber_t _fiber1;
static fiber_t _fiber2;

static void __used __noinline
_fiber_entry1(void)
{
        static uint32_t count = 0;

        while (true) {
                count += 2;
                dbgio_printf("[1;1H[1JFiber 1 called %i times\n", count);

                fiber_yield(&_fiber2);
        }
}

static void __used __noinline
_fiber_entry2(void)
{
        static uint32_t count = 1;

        while (true) {
                count += 2;
                dbgio_printf("[2;1H[1JFiber 2 called %i times\n", count);

                fiber_yield(NULL);
        }
}

int
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        fiber_init();
        fiber_stack_allocator_set(memalign, free);

        fiber_fiber_init(&_fiber1, 256, _fiber_entry1);
        fiber_fiber_init(&_fiber2, 256, _fiber_entry2);

        while (true) {
                fiber_yield(&_fiber1);

                dbgio_flush();
                vdp2_sync();
                vdp2_sync_wait();
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 3));

        vdp2_tvmd_display_set();
}
