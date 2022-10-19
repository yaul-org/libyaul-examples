/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>

static void _master_entry(void);
static void _slave_entry(void);

static uint32_t _master_counter __uncached = 0;
static uint32_t _slave_counter __uncached = 0;

int
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        _master_counter = 0;
        _slave_counter = 0;

        cpu_dual_comm_mode_set(CPU_DUAL_ENTRY_ICI);

        cpu_dual_master_set(_master_entry);
        cpu_dual_slave_set(_slave_entry);

        assert((cpu_dual_executor_get()) == CPU_MASTER);

        dbgio_printf("Master stack address: 0x%08lX\n",
            (uintptr_t)cpu_dual_master_stack_get());

        dbgio_printf("Slave stack address: 0x%08lX\n",
            (uintptr_t)cpu_dual_slave_stack_get());

        dbgio_puts("Ping ponging between master and slave\n");

        uint32_t counter;

        do {
                cpu_dual_slave_notify();

                cpu_sync_spinlock(0); {
                        counter = _slave_counter;
                } cpu_sync_spinlock_clear(0);
        } while (counter < 10);

        dbgio_printf("[3;1H[0J"
                     "Master responded %lu times\n"
                     "Slave responded  %lu times\n",
                     _master_counter,
                     _slave_counter);

        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }

        return 0;
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

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
        assert((cpu_dual_executor_get()) == CPU_SLAVE);

        cpu_sync_spinlock(0); {
                if (_slave_counter >= 10) {
                        cpu_sync_spinlock_clear(0);

                        return;
                }

                _slave_counter++;
        } cpu_sync_spinlock_clear(0);

        cpu_dual_master_notify();
}
