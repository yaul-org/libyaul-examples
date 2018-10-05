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

static void _vblank_in_handler(void);

static void _hardware_init(void);

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        char buffer[41];
        uint32_t p;
        p = 0;
        uint32_t dir;
        dir = 1;

        while (true) {
                vdp2_tvmd_vblank_out_wait();

                cons_buffer("[1;1H");

                cons_buffer(buffer);

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

                vdp2_tvmd_vblank_in_wait();
                cons_flush();
                vdp2_commit();
        }
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

        scu_dma_init();
}

static void
_vblank_in_handler(void)
{
        dma_queue_flush(DMA_QUEUE_TAG_VBLANK_IN);
}
