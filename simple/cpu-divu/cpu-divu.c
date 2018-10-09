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

        vdp2_sync_commit();
        /* cons_flush() needs to be called during VBLANK-IN */
        cons_flush();
        vdp_sync(0);

        while (true) {
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}
