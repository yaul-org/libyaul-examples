/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdlib.h>

int
main(void)
{
        dbgio_dev_default_init(DBGIO_DEV_USB_CART);
        dbgio_dev_font_load();

        for (uint32_t i = 0; i < (1 * 60); i++) {
                dbgio_flush();
        }

        uint16_t r;
        r = 0;

        while (true) {
                dbgio_printf("Hello 0x%02X\n", r);

                vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
                    COLOR_RGB1555(1, 0, r, 15));

                r++;
                r &= 0x1F;


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

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp2_tvmd_display_set();
}
