/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

static void _copy_cpd(uint8_t *cpd);
static void _copy_color_palette(color_rgb1555_t *pal);
static void _copy_map(const vdp2_scrn_cell_format_t *format);

int
main(void)
{
        const vdp2_scrn_cell_format_t format = {
                .scroll_screen     = VDP2_SCRN_NBG0,
                .cc_count          = VDP2_SCRN_CCC_PALETTE_16,
                .character_size    = 1 * 1,
                .pnd_size          = 1,
                .auxiliary_mode    = 1,
                .plane_size        = 2 * 2,
                .cp_table          = (uint32_t)VDP2_VRAM_ADDR(0, 0x00000),
                .color_palette     = (uint32_t)VDP2_CRAM_MODE_0_OFFSET(0, 0, 0),
                .map_bases.plane_a = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000),
                .map_bases.plane_b = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000),
                .map_bases.plane_c = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000),
                .map_bases.plane_d = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000)
        };

        const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_PNDR_NBG0,
                .pt[0].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[1].t0 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[2].t0 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[3].t0 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS
        };

        vdp2_vram_cycp_set(&vram_cycp);

        vdp2_scrn_cell_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* transparent = */ false);

        _copy_cpd((uint8_t *)format.cp_table);
        _copy_color_palette((color_rgb1555_t *)format.color_palette);
        _copy_map(&format);

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }
}

void
user_init(void)
{
        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(2, 0x01FFFE),
            COLOR_RGB1555(1, 0, 0, 7));

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);
}

static void
_copy_cpd(uint8_t *cpd)
{
        (void)memset(cpd + 0x00, 0x00, 0x20);
        (void)memset(cpd + 0x20, 0x11, 0x20);
}

static void
_copy_color_palette(color_rgb1555_t *color_palette)
{
        color_palette[0] = COLOR_RGB1555(1, 31, 31, 31);
        color_palette[1] = COLOR_RGB1555(1, 31,  0,  0);
}

static void
_copy_map(const vdp2_scrn_cell_format_t *format)
{
        const uint32_t page_width = VDP2_SCRN_CALCULATE_PAGE_WIDTH(format);
        const uint32_t page_height = VDP2_SCRN_CALCULATE_PAGE_HEIGHT(format);
        const uint32_t page_size = VDP2_SCRN_CALCULATE_PAGE_SIZE(format);

        uint16_t * const planes[4] = {
                (uint16_t *)format->map_bases.plane_a,
                (uint16_t *)format->map_bases.plane_b,
                (uint16_t *)format->map_bases.plane_c,
                (uint16_t *)format->map_bases.plane_d
        };

        uint16_t * const a_pages[4] = {
                &planes[0][0],
                &planes[0][1 * (page_size / 2)],
                &planes[0][2 * (page_size / 2)],
                &planes[0][3 * (page_size / 2)]
        };

        uint16_t tile;
        tile = 0;

        for (uint32_t page_y = 0; page_y < page_height; page_y++) {
                for (uint32_t page_x = 0; page_x < page_width; page_x++) {
                        uint16_t page_idx;
                        page_idx = page_x + (page_width * page_y);

                        uint16_t pnd;
                        pnd = VDP2_SCRN_PND_CONFIG_1(1, (uint32_t)format->cp_table,
                            (uint32_t)format->color_palette);

                        a_pages[0][page_idx] = pnd | tile;

                        tile ^= 1;
                }

                tile ^= 1;
        }
}
