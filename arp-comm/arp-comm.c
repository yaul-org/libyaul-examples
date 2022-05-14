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
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        if (!(arp_detect())) {
                dbgio_puts("No ARP cartridge detected!\n");
                dbgio_flush();
                vdp2_sync();
                vdp2_sync_wait();

                abort();
        }

        const char * const version_str = arp_version_string_get();

        const uint16_t vendor_id = flash_vendor_get();
        const uint16_t device_id = flash_device_get();

        dbgio_printf("Vendor:Device = %04X:%04X\n"
                     "       String = \"%s\"\n",
                     vendor_id,
                     device_id,
                     version_str);

        /* Register callback */
        arp_function_callback_set(&_arp_callback);

        uint32_t frame_count;
        frame_count = 0;

        while (true) {
                arp_function_nonblock();

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

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
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
