/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static const char *_function[] = {
        "INVALID",
        "DOWNLOAD",
        "UPLOAD",
        "EXECUTE"
};

static void _arp_callback(const arp_callback_t *callback);

int
main(void)
{
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        char arp_version[ARP_VERSION_STRING_LEN];

        arp_version_get(arp_version);

        if (*arp_version == '\0') {
                dbgio_puts("No ARP cartridge detected!\n");
                abort();
        }

        dbgio_printf("ARP version \"%s\" detected!\n", arp_version);

        /* Register callback */
        arp_function_callback_set(&_arp_callback);

        uint32_t frame_count;
        frame_count = 0;

        while (true) {
                arp_nonblock_function();

                dbgio_printf("[4;1H[1JReady... %lu\n", frame_count);

                dbgio_flush();
                vdp2_sync();
                vdp2_sync_wait();

                frame_count++;
        }

        return 0;
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

static void
_arp_callback(const arp_callback_t *callback)
{
        dbgio_printf("[6;1H[1JCallback\n"
                     "function: %s\n"
                     "ptr:      0x%08X\n"
                     "len:      0x%08X\n",
                     _function[callback->function_type],
                     callback->ptr,
                     callback->len);
        dbgio_flush();
}
