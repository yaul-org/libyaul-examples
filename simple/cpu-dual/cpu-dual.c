/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>

static void _hardware_init(void);

static void _vblank_in_handler(irq_mux_handle_t *);

static void _master_entry(void);
static void _slave_entry(void);

static char _buffer[64];
static uint32_t _master_counter = 0;
static uint32_t _slave_counter = 0;

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        _buffer[0] = '\0';
        _slave_counter = 0;

        cpu_dual_master_set(_master_entry);
        cpu_dual_slave_set(CPU_DUAL_ENTRY_ICI, _slave_entry);

        cpu_intc_mask_set(0);

        cons_buffer("Ping ponging between master and slave\n");

        while (true) {
                cpu_sync_spinlock(0); {
                        cpu_cache_purge_line(&_slave_counter);

                        (void)sprintf(_buffer, "[3;1H[0J"
                            "Master responded %lu times\n"
                            "Slave responded  %lu times\n", _master_counter, _slave_counter);
                        cons_buffer(_buffer);

                        if (_slave_counter >= 10) {
                                break;
                        }

                        cpu_dual_slave_notify();
                } cpu_sync_spinlock_clear(0);
        }

        while (true) {
                vdp2_tvmd_vblank_out_wait();

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

        irq_mux_t *vblank_in;
        vblank_in = vdp2_tvmd_vblank_in_irq_get();
        irq_mux_handle_add(vblank_in, _vblank_in_handler, NULL);

        /* Enable interrupts */
        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        vdp2_commit();
}

static void
_master_entry(void)
{
        _master_counter++;
}

static void
_slave_entry(void)
{
        cpu_sync_spinlock(0); {
                cpu_cache_purge_line(&_slave_counter);

                _slave_counter++;

                cpu_cache_purge_line(&_slave_counter);
        } cpu_sync_spinlock_clear(0);

        cpu_dual_master_notify();
}
