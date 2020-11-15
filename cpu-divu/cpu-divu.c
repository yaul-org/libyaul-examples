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

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        cpu_divu_fix16_set(FIX16(-2000.0f), FIX16(0.5f));

        /* Do something that takes up at least CPU 39 cycles */
        char *text;
        text = malloc(512);

        fix16_t quotient;
        quotient = cpu_divu_quotient_get();

        fix16_str(quotient, text, 7);
        dbgio_puts(text);
        dbgio_puts("\n");

        dbgio_flush();
        vdp_sync();

        free(text);

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
            COLOR_RGB1555(1, 0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}
