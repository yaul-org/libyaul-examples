/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>

static void _hardware_init(void);

static void _master_entry(void);
static void _slave_entry(void);

static uint32_t _master_counter __section (".uncached") = 0;
static uint32_t _slave_counter __section (".uncached") = 0;

int
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2);
        dbgio_dev_set(DBGIO_DEV_VDP2);

        _master_counter = 0;
        _slave_counter = 0;

        cpu_dual_init(CPU_DUAL_ENTRY_ICI);

        cpu_dual_master_set(_master_entry);
        cpu_dual_slave_set(_slave_entry);

        cpu_intc_mask_set(0);

        dbgio_buffer("Ping ponging between master and slave\n");

        uint32_t counter;

        do {
                cpu_dual_slave_notify();

                cpu_sync_spinlock(0); {
                        counter = _slave_counter;
                } cpu_sync_spinlock_clear(0);
        } while (counter < 10);

        char buffer[64];

        (void)sprintf(buffer,
            "[3;1H[0J"
            "Master responded %lu times\n"
            "Slave responded  %lu times\n", _master_counter, _slave_counter);
        dbgio_buffer(buffer);

        dbgio_flush();
        vdp_sync(0);

        while (true) {
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_master_entry(void)
{
        if (_master_counter >= 10) {
                return;
        }

        _master_counter++;
}

static void
_slave_entry(void)
{
        cpu_sync_spinlock(0); {
                if (_slave_counter >= 10) {
                        cpu_sync_spinlock_clear(0);

                        return;
                }

                _slave_counter++;
        } cpu_sync_spinlock_clear(0);

        cpu_dual_master_notify();
}
