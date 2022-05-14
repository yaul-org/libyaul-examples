/*
 * Copyright (c) 2012-2017 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define CPU_WDT_INTERRUPT_PRIORITY_LEVEL 8

static void _cpu_wdt_handler(void);
static void _vblank_in_handler(void *work);

void
main(void)
{
        cpu_intc_mask_set(0);

        vdp_sync_vblank_in_set(_vblank_in_handler, NULL);

        cpu_wdt_init(CPU_WDT_CLOCK_DIV_4096);
        cpu_wdt_interrupt_priority_set(8);
        cpu_wdt_timer_mode_set(CPU_WDT_MODE_INTERVAL, _cpu_wdt_handler);
        cpu_wdt_enable();

        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
                uint8_t count;
                count = cpu_wdt_count_get();

                dbgio_printf("[H[2Jcpu_wdt_count_get(): %i\n", count);
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
            COLOR_RGB1555(1, 0, 3, 3));

        vdp2_tvmd_display_set();
}

static void __used
_cpu_wdt_handler(void)
{
        assert(false);
}

static void
_vblank_in_handler(void *work __unused)
{
        cpu_wdt_count_set(0);
}
