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

static char _buffer[16];

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        cpu_slave_entry_set(CPU_MASTER, _master_entry);
        cpu_slave_entry_set(CPU_SLAVE, _slave_entry);

        cons_buffer("Notifying slave CPU and receiving notification from slave via interrupt\n\n");

        cpu_intc_mask_set(0);
        cpu_slave_notify(CPU_SLAVE);

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

static void __unused
_master_entry(void)
{
        cpu_cache_purge_line(_buffer);

        cons_buffer("Message from slave: ");
        cons_buffer(_buffer);
}

static void
_slave_entry(void)
{
        cpu_cache_purge_line(_buffer);

        sprintf(_buffer, "Hello from slave!");

        cpu_slave_notify(CPU_MASTER);
}
