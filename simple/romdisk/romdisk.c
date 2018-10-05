/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

extern uint8_t root_romdisk[];

static void _vblank_in_handler(void);

static void _delay(uint16_t);

int
main(void)
{
        vdp2_init();
        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(2, 0x01FFFE),
            COLOR_RGB555(0, 0, 7));

        vdp2_sprite_type_set(0);
        vdp2_sprite_priority_set(0, 0);

        scu_ic_mask_chg(IC_MASK_ALL, IC_MASK_VBLANK_IN);
        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_IN, _vblank_in_handler);
        scu_ic_mask_chg(~(IC_MASK_VBLANK_IN), IC_MASK_NONE);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();

        smpc_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        cons_buffer("\n[1;44m          *** ROMDISK Test ***          [m\n\n");
        cons_buffer("Mounting ROMDISK... ");

        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);

        _delay(1);

        if (romdisk != NULL) {
                cons_buffer("OK!\n");
        }

        _delay(1);

        cons_buffer("Opening \"/tmp/txt/hello.world\"... ");

        void *fh;
        if ((fh = romdisk_open(romdisk, "/tmp/txt/hello.world")) == NULL) {
                cons_buffer("[1;31mFAILED[0m\n");
                abort();
        }

        _delay(1);

        cons_buffer("OK!\n");

        ssize_t msg_len;
        msg_len = romdisk_total(fh);

        char *msg;
        msg = (char *)malloc(msg_len + 1);
        assert(msg != NULL);
        memset(msg, '\0', msg_len + 1);

        _delay(1);

        cons_buffer("Reading... ");
        msg_len = romdisk_read(fh, msg, romdisk_total(fh));

        _delay(1);

        cons_buffer("OK!\n");

        if (msg_len == 0) {
                cons_buffer("FAILED\n");
                abort();
        }

        cons_buffer("\n[1;32m");
        cons_buffer(msg);
        cons_buffer("[m");

        cons_buffer("\nTest complete!\n");

        while (true) {
                vdp2_tvmd_vblank_out_wait();
                vdp2_tvmd_vblank_in_wait();
                cons_flush();
                vdp2_commit();
        }
}

static void
_delay(uint16_t t)
{
        uint16_t frame;

        for (frame = 0; frame < (60 * t); frame++) {
                vdp2_tvmd_vblank_out_wait();
                vdp2_tvmd_vblank_in_wait();
                cons_flush();
                vdp2_commit();
        }
}

static void
_vblank_in_handler(void)
{
        dma_queue_flush(DMA_QUEUE_TAG_VBLANK_IN);
}
