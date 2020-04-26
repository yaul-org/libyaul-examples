/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define AGNES_IMPLEMENTATION
#define AGNES_SINGLE_HEADER

#include "agnes.h"

#include "game.inc"

static void _hardware_init(void);

static void _vblank_out_handler(void);

static void _input_process(agnes_input_t *);

static agnes_t _agnes;

int
main(void) {
    _hardware_init();

    agnes_load_ines_data(&_agnes, const_cast<uint8_t*>(game_data), game_size);

    while (true) {
        agnes_input_t input;
        _input_process(&input);

        agnes_set_input(&_agnes, &input, NULL);
        agnes_next_frame(&_agnes);

        for (uint32_t y = 0; y < AGNES_SCREEN_HEIGHT; y++) {
            for (uint32_t x = 0; x < AGNES_SCREEN_WIDTH; x++) {
                const agnes_color_t nes_rgb = agnes_get_screen_pixel(&_agnes, x, y);

                color_rgb1555_t color;
                color.r = nes_rgb.r;
                color.g = nes_rgb.g;
                color.b = nes_rgb.b;

                const uint32_t vram_offset = 2 * ((y * 512) + x + 32);
                volatile uint16_t * const vram =
                    (volatile uint16_t *)(VDP2_VRAM_ADDR(0, 0x00000) + vram_offset);

                *vram = color.raw;
            }
        }

        vdp_sync();
    }

    return 0;
}

static void _hardware_init(void) {
    vdp2_tvmd_display_clear();

    vdp2_scrn_bitmap_format_t format;
    memset(&format, 0x00, sizeof(format));

    format.scroll_screen = VDP2_SCRN_NBG0;
    format.cc_count = VDP2_SCRN_CCC_RGB_32768;
    format.bitmap_size.width = 512;
    format.bitmap_size.height = 256;
    format.color_palette = 0x00000000;
    format.bitmap_pattern = VDP2_VRAM_ADDR(0, 0x00000);
    format.rp_mode = 0;
    format.sf_type = VDP2_SCRN_SF_TYPE_NONE;
    format.sf_code = VDP2_SCRN_SF_CODE_A;
    format.sf_mode = 0;

    vdp2_scrn_bitmap_format_set(&format);
    vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
    vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* no_trans = */ false);

    vdp2_vram_cycp_t vram_cycp;

    vram_cycp.pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vram_cycp.pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
    vram_cycp.pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vram_cycp.pt[2].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vram_cycp.pt[3].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
    vram_cycp.pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

    vdp2_vram_cycp_set(&vram_cycp);

    vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE), COLOR_RGB1555(1, 0, 0, 0));

    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
        VDP2_TVMD_VERT_240);

    vdp_sync_vblank_out_set(_vblank_out_handler);

    vdp2_tvmd_display_set();
}

static void _vblank_out_handler(void) {
    smpc_peripheral_intback_issue();
}

static void _input_process(agnes_input_t *out_input) {
     smpc_peripheral_digital_t digital;

     smpc_peripheral_process();
     smpc_peripheral_digital_port(1, &digital);

     (void)memset(out_input, 0x00, sizeof(agnes_input_t));

     if (digital.pressed.raw == 0xFF) {
         return;
     }

     out_input->a = ((digital.pressed.button.a) != 0);
     out_input->b = ((digital.pressed.button.b) != 0);

     out_input->left = ((digital.pressed.button.left) != 0);
     out_input->right = ((digital.pressed.button.right) != 0);
     out_input->up = ((digital.pressed.button.up) != 0);
     out_input->down = ((digital.pressed.button.down) != 0);

     out_input->start = ((digital.pressed.button.start) != 0);
     out_input->select = ((digital.pressed.button.l) != 0);
     out_input->select = ((digital.pressed.button.r) != 0);
}
