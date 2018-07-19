/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

static void vblank_in_handler(irq_mux_handle_t *);

static void hardware_init(void);

static void copy_character_pattern_data(const struct scrn_cell_format *);
static void copy_color_palette(const struct scrn_cell_format *);
static void copy_map(const struct scrn_cell_format *);

int
main(void)
{
        hardware_init();

        struct scrn_cell_format format;

        format.scf_scroll_screen = SCRN_NBG0;
        format.scf_cc_count = SCRN_CCC_PALETTE_16;
        format.scf_character_size = 1 * 1;
        format.scf_pnd_size = 1;
        format.scf_auxiliary_mode = 1;
        format.scf_plane_size = 2 * 2;
        format.scf_cp_table = (uint32_t)VRAM_ADDR_4MBIT(0, 0x00000);
        format.scf_color_palette = (uint32_t)CRAM_MODE_0_OFFSET(0, 0, 0);
        format.scf_map.plane_a = (uint32_t)VRAM_ADDR_4MBIT(0, 0x08000);
        format.scf_map.plane_b = (uint32_t)VRAM_ADDR_4MBIT(0, 0x08000);
        format.scf_map.plane_c = (uint32_t)VRAM_ADDR_4MBIT(0, 0x08000);
        format.scf_map.plane_d = (uint32_t)VRAM_ADDR_4MBIT(0, 0x08000);

        struct vram_ctl *vram_ctl;
        vram_ctl = vdp2_vram_control_get();

        vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_PNDR_NBG0;
        vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[1].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[3].t0 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        copy_character_pattern_data(&format);
        copy_color_palette(&format);
        copy_map(&format);

        vdp2_vram_control_set(vram_ctl);

        vdp2_scrn_cell_format_set(&format);
        vdp2_scrn_priority_set(SCRN_NBG0, 7);
        vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ false);

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A, TVMD_VERT_224);
        vdp2_tvmd_display_set();

        while (true) {
                vdp2_tvmd_vblank_out_wait();
                vdp2_tvmd_vblank_in_wait();
        }
}

static void
hardware_init(void)
{
        /* VDP2 */
        vdp2_init();
        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(2, 0x01FFFE), COLOR_RGB555(0, 0, 7));

        /* VDP1 */
        vdp1_init();

        /* SMPC */
        smpc_init();
        smpc_peripheral_init();

        cpu_intc_mask_set (15); {
                irq_mux_t *vblank_in;

                vblank_in = vdp2_tvmd_vblank_in_irq_get();
                irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);
        } cpu_intc_mask_set(0);
}

static void
vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        vdp2_commit();
}

static void
copy_character_pattern_data(const struct scrn_cell_format *format)
{
        uint8_t *cpd;
        cpd = (uint8_t *)format->scf_cp_table;

        memset(cpd + 0x00, 0x00, 0x20);
        memset(cpd + 0x20, 0x11, 0x20);
}

static void
copy_color_palette(const struct scrn_cell_format *format)
{
        uint16_t *color_palette;
        color_palette = (uint16_t *)format->scf_color_palette;

        color_palette[0] = COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(255, 255, 255);
        color_palette[1] = COLOR_RGB_DATA | COLOR_RGB888_TO_RGB555(255,   0,   0);
}

static void
copy_map(const struct scrn_cell_format *format)
{
        uint32_t page_width;
        page_width = SCRN_CALCULATE_PAGE_WIDTH(format);
        uint32_t page_height;
        page_height = SCRN_CALCULATE_PAGE_HEIGHT(format);
        uint32_t page_size;
        page_size = SCRN_CALCULATE_PAGE_SIZE(format);

        uint16_t *planes[4];
        planes[0] = (uint16_t *)format->scf_map.plane_a;
        planes[1] = (uint16_t *)format->scf_map.plane_b;
        planes[2] = (uint16_t *)format->scf_map.plane_c;
        planes[3] = (uint16_t *)format->scf_map.plane_d;

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
                        pnd = SCRN_PND_CONFIG_1((uint32_t)format->scf_cp_table,
                            (uint32_t)format->scf_color_palette);

                        a_pages[0][page_idx] = pnd | num;

                        num ^= 1;
                }

                num ^= 1;
        }
}
