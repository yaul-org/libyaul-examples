/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>

static void _hardware_init(void);

static void _test_csi_action(const char);

int
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);

        _test_csi_action('H');

        dbgio_flush();
        vdp_sync(0);

        gdb_init();

        while (true) {
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_test_csi_action_h(void)
{
        dbgio_buffer("[28;36HHello, World!\n");

        while (true) {
                dbgio_flush();
                vdp_sync(0);
        }
}

static void
_test_csi_action(const char ch)
{
        switch (ch) {
                case 'H':
                        _test_csi_action_h();
                        break;
        }
}
