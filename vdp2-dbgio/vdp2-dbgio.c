/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdlib.h>

static void _hardware_init(void);

static void _test_csi_action(const char) __used;
static void _test_buffer_overflow(void) __used;
static void _test_cursor_position(void) __used;
static void _test_clearing(void) __used;

int
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        /* _test_csi_action('H'); */
        _test_buffer_overflow();
        /* _test_cursor_position(); */
        /* _test_clearing(); */

        while (true) {
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

static void
_test_csi_action_h(void)
{
        dbgio_buffer("[28;36HHello, World!\n");

        while (true) {
                dbgio_flush();
                vdp_sync();
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

static void
_test_buffer_overflow(void)
{
        char *large_buffer;
        large_buffer = malloc(2048);
        assert(large_buffer != NULL);

        (void)memset(large_buffer, 'x', 2048);
        large_buffer[2047] = '\0';

        /* Completely overflow the screen */
        dbgio_buffer(large_buffer);
        /* Test if we can move the cursor back to the top left */
        dbgio_buffer("[1;1HBuffer overflow averted\n");

        free(large_buffer);

        while (true) {
                dbgio_flush();
                vdp_sync();
        }
}

static void
_test_cursor_position(void)
{
        /* Move the cursor back to the top left and move the cursor down */
        dbgio_buffer("[H\n");
        for (uint32_t i = 0; i < 1024; i++) {
                dbgio_buffer("\n");
        }
        dbgio_buffer("Exceeded screen bounds\n");
        dbgio_buffer("[1;1HCursor out of bounds averted\n");

        /* Allow setting the cursor beyond the bounds of the screen */
        dbgio_buffer("[50;50H");
        dbgio_buffer("Exceeded cons screen bounds\n");
        dbgio_buffer("[2;1HCursor out of bounds averted\n");

        while (true) {
                dbgio_flush();
                vdp_sync();
        }
}

static void
_test_clearing(void)
{
}
