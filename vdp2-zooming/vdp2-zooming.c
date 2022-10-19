/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 * shazz <shazz@trsi.de>
 */

#include <yaul.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define NBG1_CPD         VDP2_VRAM_ADDR(2, 0x00000)
#define NBG1_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define NBG1_MAP_PLANE_A VDP2_VRAM_ADDR(0, 0x00000)
#define NBG1_MAP_PLANE_B VDP2_VRAM_ADDR(0, 0x08000)
#define NBG1_MAP_PLANE_C VDP2_VRAM_ADDR(0, 0x10000)
#define NBG1_MAP_PLANE_D VDP2_VRAM_ADDR(0, 0x18000)

static void _cpd_transfer(void);
static void _pal_transfer(void);
static void _pnd_transfer(uint32_t page_width, uint32_t page_height, uint32_t page_size);

static void _pnd_fill(uint16_t *map, uint16_t page_width, uint16_t page_height, uint16_t tile);

static const rgb1555_t _palette[] __unused = {
        RGB888_RGB1555_INITIALIZER(1,   0,   0,   0),
        RGB888_RGB1555_INITIALIZER(1,   0,   0, 170),
        RGB888_RGB1555_INITIALIZER(1,   0, 170,   0),
        RGB888_RGB1555_INITIALIZER(1,   0, 170, 170),
        RGB888_RGB1555_INITIALIZER(1,  85,  85,  85),
        RGB888_RGB1555_INITIALIZER(1,  85,  85, 255),
        RGB888_RGB1555_INITIALIZER(1,  85, 255,  85),
        RGB888_RGB1555_INITIALIZER(1,  85, 255, 255),
        RGB888_RGB1555_INITIALIZER(1, 170,   0,   0),
        RGB888_RGB1555_INITIALIZER(1, 170,   0, 170),
        RGB888_RGB1555_INITIALIZER(1, 170,  85,   0),
        RGB888_RGB1555_INITIALIZER(1, 170, 170, 170),
        RGB888_RGB1555_INITIALIZER(1, 255,  85,  85),
        RGB888_RGB1555_INITIALIZER(1, 255,  85, 255),
        RGB888_RGB1555_INITIALIZER(1, 255, 255,  85),
        RGB888_RGB1555_INITIALIZER(1, 255, 255, 255)
};

int
main(void)
{
        fix16_t scroll_x;
        scroll_x = FIX16(0.0f);

        fix16_t scroll_y;
        scroll_y = FIX16(0.0f);

        fix16_t zoom;
        zoom = VDP2_SCRN_REDUCTION_MIN;

        int32_t zoom_dir;
        zoom_dir = 1;

        while (true) {
                vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG1, scroll_x);
                vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG1, scroll_y);

                vdp2_scrn_reduction_x_set(VDP2_SCRN_NBG1, zoom);
                vdp2_scrn_reduction_y_set(VDP2_SCRN_NBG1, zoom);

                scroll_x = scroll_x + FIX16(16.0f);
                scroll_y = scroll_y + FIX16(16.0f);

                zoom = zoom + (zoom_dir * FIX16(0.125f));

                if (zoom >= VDP2_SCRN_REDUCTION_MAX) {
                        zoom = VDP2_SCRN_REDUCTION_MAX;
                        zoom_dir = -1;
                } else if (zoom <= VDP2_SCRN_REDUCTION_MIN) {
                        zoom = VDP2_SCRN_REDUCTION_MIN;
                        zoom_dir = 1;
                }

                vdp2_sync();
                vdp2_sync_wait();
        }
}

void
user_init(void)
{
        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(2, 0x01FFFE),
            RGB1555(1, 0, 0, 7));

        const vdp2_scrn_cell_format_t format = {
                .scroll_screen  = VDP2_SCRN_NBG1,
                .cc_count       = VDP2_SCRN_CCC_PALETTE_16,
                .character_size = 1 * 1,
                .pnd_size       = 1,
                .auxiliary_mode = 1,
                .plane_size     = 2 * 2,
                .cp_table       = NBG1_CPD,
                .color_palette  = NBG1_PAL,
                .map_bases      = {
                        .planes = {
                                NBG1_MAP_PLANE_A,
                                NBG1_MAP_PLANE_B,
                                NBG1_MAP_PLANE_C,
                                NBG1_MAP_PLANE_D
                        }
                }
        };

        vdp2_scrn_cell_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG1, 6);
        vdp2_scrn_display_set(VDP2_SCRN_NBG1_DISP);

        vdp2_vram_cycp_bank_t vram_cycp_bank[2];

        vram_cycp_bank[0].t0 = VDP2_VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank[0].t1 = VDP2_VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank[0].t2 = VDP2_VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank[0].t3 = VDP2_VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp_bank[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_bank_set(0, &vram_cycp_bank[0]);
        vdp2_vram_cycp_bank_clear(1);
        vdp2_vram_cycp_bank_set(2, &vram_cycp_bank[1]);
        vdp2_vram_cycp_bank_clear(3);

        vdp2_scrn_reduction_set(VDP2_SCRN_NBG0, VDP2_SCRN_REDUCTION_QUARTER);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        _cpd_transfer();
        _pal_transfer();

        const uint32_t page_width = VDP2_SCRN_CALCULATE_PAGE_WIDTH(&format);
        const uint32_t page_height = VDP2_SCRN_CALCULATE_PAGE_HEIGHT(&format);
        const uint32_t page_size = VDP2_SCRN_CALCULATE_PAGE_SIZE(&format);

        _pnd_transfer(page_width, page_height, page_size);

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();
}

static void
_cpd_transfer(void)
{
        for (uint16_t tile = 0; tile < 16; tile++) {
                const uint8_t byte = (tile << 4) | tile;

                (void)memset((void *)(NBG1_CPD | (tile << 5)), byte, 32);
        }
}

static void
_pal_transfer(void)
{
        (void)memcpy((void *)NBG1_PAL, _palette, sizeof(_palette));
}

static void
_pnd_transfer(uint32_t page_width, uint32_t page_height, uint32_t page_size)
{
        uint16_t * const map = malloc(page_size);
        assert(map != NULL);

        uint32_t pnd;
        pnd = NBG1_MAP_PLANE_A;

        for (uint32_t tile = 0; tile < 16; tile++) {
                _pnd_fill(map, page_width, page_height, tile);

                scu_dma_transfer(0, (void *)pnd, map, page_size);

                pnd += page_size;
        }

        free(map);
}

static void
_pnd_fill(uint16_t *map, uint16_t page_width, uint16_t page_height, uint16_t tile)
{
        for (uint16_t x = 0; x < page_width; x++) {
                for (uint16_t y = 0; y < page_height; y++) {
                        const uint32_t cpd = (uint32_t)NBG1_CPD | (tile << 5);
                        const uint16_t pnd = VDP2_SCRN_PND_CONFIG_1(1, cpd, NBG1_PAL);

                        map[x + (y * page_width)] = pnd;
                }
        }
}
