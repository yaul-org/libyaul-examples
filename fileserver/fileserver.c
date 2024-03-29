/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>

int
main(void)
{
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        /* Point the root path to where this example is located */
        fileclient_sector_request("fileserver.c", 0, (void *)LWRAM(0));

        uint32_t sector_count;
        sector_count = fileclient_sector_count_request("fileserver.c");

        dbgio_printf("Sector count: %lu\n", sector_count);
        dbgio_puts((void *)LWRAM(0));

        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }
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
