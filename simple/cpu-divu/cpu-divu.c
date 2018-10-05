/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

static void _hardware_init(void);

static void _vblank_in_handler(void);

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        /* Shift dividend by 16 bits (32-bit to 64-bit value)
         *
         * r0 = 0x7FFE8000      -> 0x00007FFE:0x80000000
         *
         * r1 = swap.w(r0)      -> 0x80007FFE
         * r1 = exts.w(r1)      -> 0xFFFF7FFE
         * Write r1 to DVDNTH
         * r0 = r0 << 16        -> 0x80000000
         * Write r0 to DVDNTL
         */

        uint32_t dividend;
        dividend = 0x7FFFFFFF;
        uint32_t divisor;
        divisor = 0x00010000;

        uint32_t dh;
        dh = cpu_instr_swapw(dividend);
        uint32_t dl;
        dl = dividend << 16;

        cpu_divu_64_32_set(dh, dl, divisor);

        /* Do something that takes up at least CPU 39 cycles */
        char *text;
        text = malloc(512);

        uint32_t quotient;
        quotient = cpu_divu_quotient_get();

        fix16_to_str((fix16_t)quotient, text, 7);
        cons_buffer(text);
        cons_buffer("\n");

        while (true) {
                vdp2_tvmd_vblank_out_wait();

                vdp2_tvmd_vblank_in_wait();
                cons_flush();
                vdp2_commit();
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
