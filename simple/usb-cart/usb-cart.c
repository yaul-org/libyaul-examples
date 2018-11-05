/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>

static void _hardware_init(void);

static void _ovi_handler(void);

static volatile uint32_t _ovf_count = 0;

/* Transfer 512 KiB */
static uint8_t _copy_buffer[62] __aligned(0x100);

int
main(void)
{
        (void)memset(_copy_buffer, '\0', sizeof(_copy_buffer));
        usb_cart_dma_read(_copy_buffer, sizeof(_copy_buffer));

        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2);
        dbgio_dev_set(DBGIO_DEV_VDP2);

        dbgio_buffer("Using f/8 CPU FRT divisor\n\n");

        char buffer[256];

        uint16_t before;
        uint16_t after;
        uint32_t count;

        uint32_t i;

        cpu_frt_count_set(0);
        _ovf_count = 0;

        before = cpu_frt_count_get();
        for (i = 0; i < sizeof(_copy_buffer); i++) {
                usb_cart_byte_send(_copy_buffer[i]);
        }
        after = cpu_frt_count_get();
        count = (after - before) + (65535 * _ovf_count);

        (void)sprintf(buffer, "Using CPU byte transfer:\n%16lu FRT ticks\n", count);
        dbgio_buffer(buffer);

        cpu_frt_count_set(0);
        _ovf_count = 0;

        before = cpu_frt_count_get();
        usb_cart_dma_send(_copy_buffer, sizeof(_copy_buffer));
        after = cpu_frt_count_get();
        count = (after - before) + (65535 * _ovf_count);

        (void)sprintf(buffer, "Using CPU-DMAC:\n%16lu FRT ticks\n", count);
        dbgio_buffer(buffer);

        dbgio_buffer(".\n");

        dbgio_flush();
        vdp2_sync_commit();
        vdp_sync(0);

        dbgio_flush();
        vdp2_sync_commit();
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

        vdp2_tvmd_display_set();

        cpu_frt_init(FRT_CLOCK_DIV_8);
        cpu_frt_ovi_set(_ovi_handler);

        cpu_intc_mask_set(0);
}

static void
_ovi_handler(void)
{
        _ovf_count++;
}
