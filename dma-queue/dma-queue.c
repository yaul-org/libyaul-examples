/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <tga.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static vdp2_scrn_bitmap_format_t _bitmap_format = {
        .scroll_screen = VDP2_SCRN_NBG0,
        .ccc           = VDP2_SCRN_CCC_RGB_32768,
        .bitmap_size   = VDP2_SCRN_BITMAP_SIZE_512X256,
        .palette_base  = 0x00000000,
        .bitmap_base   = VDP2_VRAM_ADDR(0, 0x00000)
};

/* Setting the no-access timing cycles from 0xF to 0xE increased performance
 * tremendously. This may be exasperated by the fact that we're writing to VDP2
 * VRAM outside of VBLANK-IN */
static const vdp2_vram_cycp_t _vram_cycp = {
        {
                {
                        .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t4 = VDP2_VRAM_CYCP_CPU_RW,
                        .t5 = VDP2_VRAM_CYCP_CPU_RW,
                        .t6 = VDP2_VRAM_CYCP_CPU_RW,
                        .t7 = VDP2_VRAM_CYCP_CPU_RW
                }, {
                        .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t4 = VDP2_VRAM_CYCP_CPU_RW,
                        .t5 = VDP2_VRAM_CYCP_CPU_RW,
                        .t6 = VDP2_VRAM_CYCP_CPU_RW,
                        .t7 = VDP2_VRAM_CYCP_CPU_RW
                }, {
                        .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t4 = VDP2_VRAM_CYCP_CPU_RW,
                        .t5 = VDP2_VRAM_CYCP_CPU_RW,
                        .t6 = VDP2_VRAM_CYCP_CPU_RW,
                        .t7 = VDP2_VRAM_CYCP_CPU_RW
                }, {
                        .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t4 = VDP2_VRAM_CYCP_CPU_RW,
                        .t5 = VDP2_VRAM_CYCP_CPU_RW,
                        .t6 = VDP2_VRAM_CYCP_CPU_RW,
                        .t7 = VDP2_VRAM_CYCP_CPU_RW
                }
        }
};

extern uint8_t asset_bitmap1_tga[];
extern uint8_t asset_bitmap2_tga[];
extern uint8_t asset_bitmap3_tga[];
extern uint8_t asset_bitmap4_tga[];
extern uint8_t asset_bitmap5_tga[];
extern uint8_t asset_bitmap6_tga[];
extern uint8_t asset_bitmap7_tga[];
extern uint8_t asset_bitmap8_tga[];
extern uint8_t asset_bitmap9_tga[];

static const void *_tgas[] = {
        asset_bitmap1_tga,
        asset_bitmap2_tga,
        asset_bitmap3_tga,
        asset_bitmap4_tga,
        asset_bitmap5_tga,
        asset_bitmap6_tga,
        asset_bitmap7_tga,
        asset_bitmap8_tga,
        asset_bitmap9_tga,
        NULL
};

static uint8_t _intermediate_buffer[512 * 256 * 2] __aligned(4);

static uint32_t _tga_file_decode(const void *asset_tga, void *buffer);

int
main(void)
{
        vdp2_scrn_bitmap_format_set(&_bitmap_format);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);

        vdp2_vram_cycp_set(&_vram_cycp);

        const void **current_tga;
        current_tga = &_tgas[0];

        uint32_t bank;
        bank = 0;

        while (true) {
                if (*current_tga == NULL) {
                        current_tga = &_tgas[0];
                }

                void * const vram = (void *)VDP2_VRAM_ADDR(2 * bank, 0x00000);

                const uint32_t intermediate_size =
                    _tga_file_decode(*current_tga, _intermediate_buffer);

                /* Set the back screen to its original color */
                vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(2 * bank, 0x01FFFE),
                    RGB1555(1, 7, 7, 7));

                _bitmap_format.bitmap_base = VDP2_VRAM_ADDR(2 * bank, 0x00000);

                vdp2_scrn_bitmap_base_set(&_bitmap_format);

                vdp2_tvmd_vblank_in_next_wait(15);

                vdp_dma_enqueue(vram, _intermediate_buffer, intermediate_size);

                vdp2_sync();
                vdp2_sync_wait();

                current_tga++;
                bank ^= 1;
        }

        return 0;
}

void
user_init(void)
{
        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 6);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 7, 7, 7));

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_tvmd_display_set();
}

static uint32_t
_tga_file_decode(const void *asset_tga, void *buffer)
{
        assert(asset_tga != NULL);
        assert(buffer != NULL);

        tga_t tga;

        const int32_t tga_ret __unused = tga_read(&tga, asset_tga);
        assert(tga_ret == TGA_FILE_OK);

        const int32_t tga_size = tga_image_decode(&tga, buffer);

        /* XXX: The TGA library isn't very well written. The pixel count is
         *      returned, but because we're dealing with 16-BPP TGA images, it's
         *      2 bytes per pixel */
        return (2 * tga_size);
}
