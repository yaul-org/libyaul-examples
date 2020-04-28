/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

static void _hardware_init(void);

int
main(void)
{
        static const uint32_t _columns = 40;

        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        dbgio_buffer("Waiting to establish a connection with GDB\n");
        dbgio_flush();
        vdp_sync();

        /* Unmask all interrupts just to be sure GDB works */
        cpu_intc_mask_set(0);
        gdb_init();

        char buffer[_columns + 1];

        fix16_t x;
        x = F16(0.0f);

        int32_t dir;
        dir = 1;

        while (true) {
                (void)memset(buffer, ' ', sizeof(buffer));
                buffer[_columns] = '\0';

                buffer[fix16_to_int(x)] = 'o';

                x += F16(0.25f) * dir;

                if (x <= F16(0.0f)) {
                        x = F16(0.0f);

                        dir *= -1;
                } else if (x >= F16((float)_columns - 0.5f)) {
                        x = F16((float)_columns - 1.0f);

                        dir *= -1;
                }

                dbgio_buffer("[H[2J[14;1H");
                dbgio_buffer(buffer);

                dbgio_flush();
                vdp_sync();

                gdb_break();
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}
