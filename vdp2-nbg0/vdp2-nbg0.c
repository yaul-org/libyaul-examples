/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

static void _cpd_copy(uint8_t *cpd_base);
static void _palette_copy(rgb1555_t *palette_base);
static void _map_copy(const vdp2_scrn_cell_format_t *format,
    const vdp2_scrn_normal_map_t *normal_map);

int
main(void)
{
        const vdp2_scrn_cell_format_t format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .ccc           = VDP2_SCRN_CCC_PALETTE_16,
                .char_size     = VDP2_SCRN_CHAR_SIZE_1X1,
                .pnd_size      = 1,
                .aux_mode      = VDP2_SCRN_AUX_MODE_1,
                .plane_size    = VDP2_SCRN_PLANE_SIZE_2X2,
                .cpd_base      = (uint32_t)VDP2_VRAM_ADDR(0, 0x00000),
                .palette_base  = (uint32_t)VDP2_CRAM_MODE_0_OFFSET(0, 0, 0)
        };

        const vdp2_scrn_normal_map_t normal_map = {
                .plane_a = (vdp2_vram_t)VDP2_VRAM_ADDR(0, 0x08000),
                .plane_b = (vdp2_vram_t)VDP2_VRAM_ADDR(0, 0x08000),
                .plane_c = (vdp2_vram_t)VDP2_VRAM_ADDR(0, 0x08000),
                .plane_d = (vdp2_vram_t)VDP2_VRAM_ADDR(0, 0x08000)
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

        vdp2_scrn_cell_format_set(&format, &normal_map);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);

        _cpd_copy((uint8_t *)format.cpd_base);
        _palette_copy((rgb1555_t *)format.palette_base);
        _map_copy(&format, &normal_map);

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }
}

void
user_init(void)
{
        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(2, 0x01FFFE),
            RGB1555(1, 0, 0, 7));

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);
}

static void
_cpd_copy(uint8_t *cpd_base)
{
        (void)memset(cpd_base + 0x00, 0x00, 0x20);
        (void)memset(cpd_base + 0x20, 0x11, 0x20);
}

static void
_palette_copy(rgb1555_t *palette_base)
{
        palette_base[0] = RGB1555(1, 31, 31, 31);
        palette_base[1] = RGB1555(1, 31,  0,  0);
}

static void
_map_copy(const vdp2_scrn_cell_format_t *format, const vdp2_scrn_normal_map_t *normal_map)
{
        const uint32_t page_width = VDP2_SCRN_PAGE_WIDTH_CALCULATE(format);
        const uint32_t page_height = VDP2_SCRN_PAGE_HEIGHT_CALCULATE(format);
        const uint32_t page_size = VDP2_SCRN_PAGE_SIZE_CALCULATE(format);

        uint16_t * const a_pages[4] = {
                (uint16_t *)normal_map->base_addr[0],
                (uint16_t *)normal_map->base_addr[1 * (page_size / sizeof(uint16_t))],
                (uint16_t *)normal_map->base_addr[2 * (page_size / sizeof(uint16_t))],
                (uint16_t *)normal_map->base_addr[3 * (page_size / sizeof(uint16_t))]
        };

        uint16_t tile;
        tile = 0;

        for (uint32_t page_y = 0; page_y < page_height; page_y++) {
                for (uint32_t page_x = 0; page_x < page_width; page_x++) {
                        const uint16_t page_idx = page_x + (page_width * page_y);
                        const uint16_t pnd = VDP2_SCRN_PND_CONFIG_1(1, (uint32_t)format->cpd_base,
                            (uint32_t)format->palette_base);

                        a_pages[0][page_idx] = pnd | tile;

                        tile ^= 1;
                }

                tile ^= 1;
        }
}
