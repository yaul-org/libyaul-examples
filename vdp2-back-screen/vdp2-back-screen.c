/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdlib.h>

static void _vblank_out_handler(void *);

static smpc_peripheral_digital_t _digital;

void
main(void)
{
        uint16_t width __unused;
        uint16_t height;
        vdp2_tvmd_display_res_get(&width, &height);

        rgb1555_t * const buffer =
            malloc(sizeof(rgb1555_t) * height);
        assert(buffer != NULL);

        uint16_t buffer_count;
        buffer_count = 0;
        uint16_t count;
        count = 0;
        bool switch_buffer_count;
        switch_buffer_count = false;

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                if (_digital.held.button.a) {
                        switch_buffer_count ^= true;
                }

                if (switch_buffer_count) {
                        buffer_count = height;
                } else {
                        buffer_count = 1;
                }

                vdp2_scrn_back_buffer_set(VDP2_VRAM_ADDR(0, 0x00000), buffer,
                    buffer_count);
                vdp2_scrn_back_sync();

                for (uint16_t i = 0; i < buffer_count; i++) {
                        buffer[i] = RGB1555(1, i + count, i + count, i + count);
                }

                count++;

                vdp2_sync();
                vdp2_sync_wait();
        }
}

void
user_init(void)
{
        smpc_peripheral_init();

        vdp2_vram_cycp_clear();

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_tvmd_display_set();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}
