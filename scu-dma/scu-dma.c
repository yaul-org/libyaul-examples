/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <yaul.h>

static void _hardware_init(void);

int
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2);
        dbgio_dev_set(DBGIO_DEV_VDP2);

        char buffer[41];
        uint32_t p;
        p = 0;
        uint32_t dir;
        dir = 1;

        while (true) {
                vdp2_tvmd_vblank_out_wait();

                dbgio_buffer("[1;1H");

                dbgio_buffer(buffer);

                memset(buffer, ' ', sizeof(buffer));
                buffer[40] = '\0';

                buffer[p >> 16] = '*';

                if ((p >> 16) == 39) {
                        dir = -1;
                        p = 38 << 16;
                } else if ((p >> 16) == 0) {
                        dir = 1;
                        p = 1 << 16;
                } else {
                        p = p + (dir * (1 << 14));
                }

                dbgio_flush();
                vdp_sync(0);
        }
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
