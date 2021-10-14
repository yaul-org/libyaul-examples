/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

static void _copy_character_pattern_data(const vdp2_scrn_cell_format_t *);
static void _copy_color_palette(const vdp2_scrn_cell_format_t *);
static void _copy_map(const vdp2_scrn_cell_format_t *);

int
main(void)
{
        vdp2_scrn_cell_format_t format;

        format.scroll_screen = VDP2_SCRN_NBG0;
        format.cc_count = VDP2_SCRN_CCC_PALETTE_16;
        format.character_size = 1 * 1;
        format.pnd_size = 1;
        format.auxiliary_mode = 1;
        format.plane_size = 2 * 2;
        format.cp_table = (uint32_t)VDP2_VRAM_ADDR(0, 0x00000);
        format.color_palette = (uint32_t)VDP2_CRAM_MODE_0_OFFSET(0, 0, 0);
        format.map_bases.plane_a = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000);
        format.map_bases.plane_b = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000);
        format.map_bases.plane_c = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000);
        format.map_bases.plane_d = (uint32_t)VDP2_VRAM_ADDR(0, 0x08000);

        vdp2_vram_cycp_t vram_cycp;

        vram_cycp.pt[0].t0 = VDP2_VRAM_CYCP_PNDR_NBG0;
        vram_cycp.pt[0].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[1].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
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

        _copy_character_pattern_data(&format);
        _copy_color_palette(&format);
        _copy_map(&format);

        vdp2_scrn_cell_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* transparent = */ false);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);
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

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_clear();
}

static void
_copy_character_pattern_data(const vdp2_scrn_cell_format_t *format)
{
        uint8_t *cpd;
        cpd = (uint8_t *)format->cp_table;

        memset(cpd + 0x00, 0x00, 0x20);
        memset(cpd + 0x20, 0x11, 0x20);
}

static void
_copy_color_palette(const vdp2_scrn_cell_format_t *format)
{
        uint16_t *color_palette;
        color_palette = (uint16_t *)format->color_palette;

        color_palette[0] = COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(255, 255, 255);
        color_palette[1] = COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(255,   0,   0);
}

static void
_copy_map(const vdp2_scrn_cell_format_t *format)
{
        uint32_t page_width;
        page_width = VDP2_SCRN_CALCULATE_PAGE_WIDTH(format);
        uint32_t page_height;
        page_height = VDP2_SCRN_CALCULATE_PAGE_HEIGHT(format);
        uint32_t page_size;
        page_size = VDP2_SCRN_CALCULATE_PAGE_SIZE(format);

        uint16_t *planes[4];
        planes[0] = (uint16_t *)format->map_bases.plane_a;
        planes[1] = (uint16_t *)format->map_bases.plane_b;
        planes[2] = (uint16_t *)format->map_bases.plane_c;
        planes[3] = (uint16_t *)format->map_bases.plane_d;

        uint16_t *a_pages[4];
        a_pages[0] = &planes[0][0];
        a_pages[1] = &planes[0][1 * (page_size / 2)];
        a_pages[2] = &planes[0][2 * (page_size / 2)];
        a_pages[3] = &planes[0][3 * (page_size / 2)];

        uint16_t num;
        num = 0;

        uint32_t page_x;
        uint32_t page_y;
        for (page_y = 0; page_y < page_height; page_y++) {
                for (page_x = 0; page_x < page_width; page_x++) {
                        uint16_t page_idx;
                        page_idx = page_x + (page_width * page_y);

                        uint16_t pnd;
                        pnd = VDP2_SCRN_PND_CONFIG_1(1, (uint32_t)format->cp_table,
                            (uint32_t)format->color_palette);

                        a_pages[0][page_idx] = pnd | num;

                        num ^= 1;
                }

                num ^= 1;
        }
}
