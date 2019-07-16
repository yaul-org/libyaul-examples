/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdlib.h>

static void _hardware_init(void);

int
main(void)
{
        _hardware_init();

        uint16_t width __unused;
        uint16_t height;
        vdp2_tvmd_display_res_get(&width, &height);

        color_rgb555_t *buffer;
        buffer = malloc(sizeof(color_rgb555_t) * height);

        vdp2_scrn_back_screen_buffer_set(VDP2_VRAM_ADDR(0, 0x00000), buffer, height);

        uint16_t count;
        count = 0;

        while (true) {
                uint16_t i;
                for (i = 0; i < height; i++) {
                        buffer[i] = COLOR_RGB555(i + count, i + count, i + count);
                }
                count++;

                vdp_sync(0);
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_scrn_display_clear();

        vdp2_vram_cycp_clear();

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_tvmd_display_set();
}
