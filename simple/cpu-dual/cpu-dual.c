/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>

static void _hardware_init(void);

static void _vblank_in_handler(void);

static void _master_entry(void);
static void _slave_entry(void);

static uint32_t _master_counter __section (".uncached") = 0;
static uint32_t _slave_counter __section (".uncached") = 0;

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        _master_counter = 0;
        _slave_counter = 0;

        cpu_dual_init(CPU_DUAL_ENTRY_ICI);

        cpu_dual_master_set(_master_entry);
        cpu_dual_slave_set(_slave_entry);

        cpu_intc_mask_set(0);

        cons_buffer("Ping ponging between master and slave\n");

        uint32_t counter;

        do {
                cpu_dual_slave_notify();

                cpu_sync_spinlock(0); {
                        counter = _slave_counter;
                } cpu_sync_spinlock_clear(0);
        } while (counter < 10);

        char buffer[64];

        while (true) {
                vdp2_tvmd_vblank_out_wait();

                (void)sprintf(buffer,
                    "[3;1H[0J"
                    "Master responded %lu times\n"
                    "Slave responded  %lu times\n", _master_counter, _slave_counter);
                cons_buffer(buffer);

                vdp2_commit();

                vdp2_tvmd_vblank_in_wait();
                cons_flush();
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_init();

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);

        vdp2_sprite_type_set(0);
        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        scu_ic_mask_chg(IC_MASK_ALL, IC_MASK_VBLANK_IN);
        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_IN, _vblank_in_handler);
        scu_ic_mask_chg(~(IC_MASK_VBLANK_IN), IC_MASK_NONE);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_vblank_in_handler(void)
{
        dma_queue_flush(DMA_QUEUE_TAG_VBLANK_IN);
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
