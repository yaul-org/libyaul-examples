/*
 * Copyright (c) 2012-2017 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#define ASSERT                  1
#define FIXMATH_NO_OVERFLOW     1
#define FIXMATH_NO_ROUNDING     1

#include <yaul.h>

#include <stdio.h>

#include "frt.h"

static volatile uint32_t tick = 0;
static volatile uint16_t frt_tick = 0;

static char text_buffer[256];

static void hardware_init(void);
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

void
main(void)
{
        hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 30);

        while (true) {
                vdp2_tvmd_vblank_out_wait();

                // Update
                cons_buffer("[1;1H[2J");

                vdp2_tvmd_vblank_in_wait();

                ufix16_t time_ms;
                char time_str[16];

                time_ms = tim_frt_ticks_to_ms(frt_tick);
                ufix16_to_str(time_ms, time_str, 7);

                (void)sprintf(text_buffer, "%08u ticks, %08lu ticks, %sms", frt_tick, tick, time_str);
                cons_buffer(text_buffer);

                // Draw
                cons_flush();
        }
}

static void
hardware_init(void)
{
        /* VDP2 */
        vdp2_init();

        /* Disable interrupts */
        cpu_intc_disable();

        irq_mux_t *vblank_in;
        irq_mux_t *vblank_out;

        vblank_in = vdp2_tvmd_vblank_in_irq_get();
        irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);

        vblank_out = vdp2_tvmd_vblank_out_irq_get();
        irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);

        /* Enable interrupts */
        cpu_intc_enable();

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(2, 0x01FFFE),
            COLOR_RGB555(0, 0, 7));

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);
        vdp2_tvmd_display_set();

/* FRC operates on the timer drive clock (φ/4), which has a cycle of 4
 * times the system clock (φ).  For this reason, when the CPU performs an
 * access, both the CPU and FRT will be operating, so a WAIT request will
 * be generated from the FRT to the CPU. The number of access cycles thus
 * varies by between 3 and 12 cycles. */

        tim_frt_init(TIM_CKS_128);
}

static void
vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        /* if ((vdp2_tvmd_vcount_get()) >= 223) { */
        /*         frt_tick = tim_frt_get(); */
        /* } */

        frt_tick = 65535;
}

static void
vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
        if ((vdp2_tvmd_vcount_get()) == 0) {
                tick = (tick & 0xFFFFFFFF) + 1;

                tim_frt_set(0);
        }
}
