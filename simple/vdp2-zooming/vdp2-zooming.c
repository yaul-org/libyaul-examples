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

static struct smpc_peripheral_digital _digital_pad;
static uint64_t _tick = 0;
static fix16_t _scroll_x = F16(0.0f);
static fix16_t _scroll_y = F16(0.0f);
static fix16_t _zoom = F16(1.0f);

static void _hardware_init(void);

static void _update(void);
static void _draw(void);

static void _vblank_in_handler(void);
static void _vblank_out_handler(void);

int
main(void)
{
        _hardware_init();

        while (true) {
                vdp2_tvmd_vblank_out_wait();
                smpc_peripheral_digital_port(1, &_digital_pad);
                _update();

                vdp2_tvmd_vblank_in_wait();
                _draw();
                vdp2_commit();
        }
}

static void
_hardware_init(void)
{
        static color_rgb555_t palette[] __unused = {
                COLOR_RGB555(0x1F, 0x00, 0x00),
                COLOR_RGB555(0x1F, 0x1F, 0x1F),
                COLOR_RGB555(0x04, 0x14, 0x1A),
                COLOR_RGB555(0x1F, 0x0F, 0x09),
                COLOR_RGB555(0x13, 0x19, 0x1F),
                COLOR_RGB555(0x19, 0x18, 0x1F),
                COLOR_RGB555(0x1F, 0x1F, 0x14),
                COLOR_RGB555(0x10, 0x10, 0x10),
                COLOR_RGB555(0x18, 0x18, 0x18),
                COLOR_RGB555(0x04, 0x10, 0x19),
                COLOR_RGB555(0x1F, 0x07, 0x02),
                COLOR_RGB555(0x07, 0x13, 0x1B),
                COLOR_RGB555(0x14, 0x13, 0x1F),
                COLOR_RGB555(0x1F, 0x19, 0x04),
                COLOR_RGB555(0x00, 0x00, 0x00),
                COLOR_RGB555(0x10, 0x15, 0x1F)
        };

        vdp1_init();
        vdp2_sprite_type_set(0);
        vdp2_sprite_priority_set(0, 0);

        vdp2_init();
        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(2, 0x01FFFE),
            COLOR_RGB555(0, 0, 7));
        vdp2_tvmd_display_clear();

        /* SMPC */
        smpc_init();
        smpc_peripheral_init();

        scu_ic_mask_chg(IC_MASK_ALL, IC_MASK_VBLANK_IN | IC_MASK_VBLANK_OUT);
        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_IN, _vblank_in_handler);
        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_OUT, _vblank_out_handler);
        scu_ic_mask_chg(~(IC_MASK_VBLANK_IN | IC_MASK_VBLANK_OUT), IC_MASK_NONE);

        uint16_t *cpd;
        cpd = (uint16_t *)VRAM_ADDR_4MBIT(2, 0x00000);

        color_rgb555_t *color_palette;
        color_palette = (color_rgb555_t *)CRAM_MODE_1_OFFSET(0, 0, 0);

        uint16_t *planes[4];
        planes[0] = (uint16_t *)VRAM_ADDR_4MBIT(0, 0x08000);
        planes[1] = (uint16_t *)VRAM_ADDR_4MBIT(0, 0x10000);
        planes[2] = (uint16_t *)VRAM_ADDR_4MBIT(0, 0x18000);
        planes[3] = (uint16_t *)VRAM_ADDR_4MBIT(0, 0x20000);

        struct scrn_cell_format format;
        format.scf_scroll_screen = SCRN_NBG1;
        format.scf_cc_count = SCRN_CCC_PALETTE_16;
        format.scf_character_size = 1 * 1;
        format.scf_pnd_size = 1;
        format.scf_auxiliary_mode = 1;
        format.scf_plane_size = 2 * 2;
        format.scf_cp_table = (uint32_t)cpd;
        format.scf_color_palette = (uint32_t)color_palette;
        format.scf_map.plane_a = (uint32_t)planes[0];
        format.scf_map.plane_b = (uint32_t)planes[1];
        format.scf_map.plane_c = (uint32_t)planes[2];
        format.scf_map.plane_d = (uint32_t)planes[3];

        vdp2_scrn_cell_format_set(&format);
        vdp2_scrn_priority_set(SCRN_NBG1, 7);
        vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ false);

        struct vram_ctl *vram_ctl;
        vram_ctl = vdp2_vram_control_get();

        vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_PNDR_NBG1;
        vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_PNDR_NBG1;
        vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_PNDR_NBG1;
        vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_PNDR_NBG1;
        vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;

        vram_ctl->vram_cycp.pt[2].t7 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[2].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[2].t5 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[2].t4 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[2].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[2].t0 = VRAM_CTL_CYCP_NO_ACCESS;

        /* We want to be in VBLANK-IN (retrace) */
        uint32_t j;
        for (j = 0; j < 4096; j++) {
                uint32_t i;
                for (i = 0; i < 16; i++) {
                        uint32_t x;
                        x = j & 0x0F;

                        cpd[i + (j * (32 / 2))] = (x << 12) | (x << 8) | (x << 4) | x;
                }
        }

        (void)memcpy(color_palette, palette, sizeof(palette));

        uint32_t page_width;
        page_width = SCRN_CALCULATE_PAGE_WIDTH(&format);
        uint32_t page_height;
        page_height = SCRN_CALCULATE_PAGE_HEIGHT(&format);
        uint32_t page_size;
        page_size = SCRN_CALCULATE_PAGE_SIZE(&format);

        uint16_t *a_pages[4];
        a_pages[0] = &planes[0][0];
        a_pages[1] = &planes[0][1 * (page_size / 2)];
        a_pages[2] = &planes[0][2 * (page_size / 2)];
        a_pages[3] = &planes[0][3 * (page_size / 2)];

        uint16_t *b_pages[4];
        b_pages[0] = &planes[1][0];
        b_pages[1] = &planes[1][1 * (page_size / 2)];
        b_pages[2] = &planes[1][2 * (page_size / 2)];
        b_pages[3] = &planes[1][3 * (page_size / 2)];

        uint16_t *c_pages[4];
        c_pages[0] = &planes[2][0];
        c_pages[1] = &planes[2][1 * (page_size / 2)];
        c_pages[2] = &planes[2][2 * (page_size / 2)];
        c_pages[3] = &planes[2][3 * (page_size / 2)];

        uint16_t *d_pages[4];
        d_pages[0] = &planes[3][0];
        d_pages[1] = &planes[3][1 * (page_size / 2)];
        d_pages[2] = &planes[3][2 * (page_size / 2)];
        d_pages[3] = &planes[3][3 * (page_size / 2)];

        uint32_t page_y;
        for (page_y = 0; page_y < page_height; page_y++) {
                uint32_t page_x;
                for (page_x = 0; page_x < page_width; page_x++) {
                        uint16_t page_idx;
                        page_idx = page_x + (page_width * page_y);

                        a_pages[0][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 0 * 32), (uint32_t)color_palette);
                        a_pages[1][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 1 * 32), (uint32_t)color_palette);
                        a_pages[2][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 2 * 32), (uint32_t)color_palette);
                        a_pages[3][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 3 * 32), (uint32_t)color_palette);

                        b_pages[0][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 4 * 32), (uint32_t)color_palette);
                        b_pages[1][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 5 * 32), (uint32_t)color_palette);
                        b_pages[2][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 6 * 32), (uint32_t)color_palette);
                        b_pages[3][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 7 * 32), (uint32_t)color_palette);

                        c_pages[0][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 8 * 32), (uint32_t)color_palette);
                        c_pages[1][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + ( 9 * 32), (uint32_t)color_palette);
                        c_pages[2][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + (10 * 32), (uint32_t)color_palette);
                        c_pages[3][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + (11 * 32), (uint32_t)color_palette);

                        d_pages[0][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + (12 * 32), (uint32_t)color_palette);
                        d_pages[1][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + (13 * 32), (uint32_t)color_palette);
                        d_pages[2][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + (14 * 32), (uint32_t)color_palette);
                        d_pages[3][page_idx] = SCRN_PND_CONFIG_1((uint32_t)cpd + (15 * 32), (uint32_t)color_palette);
                }
        }

        vdp2_vram_control_set(vram_ctl);

        vdp2_scrn_reduction_set(SCRN_NBG0, SCRN_REDUCTION_QUARTER);

        /* Set TV display to 320x240 */
        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_240);

        /* Turn on display */
        vdp2_tvmd_display_set();
}

static void
_update(void)
{
        if (_digital_pad.connected == 0) {
                return;
        }

        if (_digital_pad.pressed.button.left) {
                _scroll_x = fix16_sub(_scroll_x, F16(4.0f));
        } else if (_digital_pad.pressed.button.right) {
                _scroll_x = fix16_add(_scroll_x, F16(4.0f));
        }

        if (_digital_pad.pressed.button.up) {
                _scroll_y = fix16_sub(_scroll_y, F16(4.0f));
        } else if (_digital_pad.pressed.button.down) {
                _scroll_y = fix16_add(_scroll_y, F16(4.0f));
        }

        if (_digital_pad.pressed.button.l) {
                _zoom = fix16_add(_zoom, SCRN_REDUCTION_STEP);
        } else if (_digital_pad.pressed.button.r) {
                _zoom = fix16_sub(_zoom, SCRN_REDUCTION_STEP);
        }

        if (_digital_pad.pressed.button.start) {
                abort();
        }

        _scroll_x = fix16_clamp(_scroll_x, F16(0.0f), _scroll_x);
        _scroll_y = fix16_clamp(_scroll_y, F16(0.0f), _scroll_y);

        _zoom = fix16_clamp(_zoom, SCRN_REDUCTION_MIN, SCRN_REDUCTION_MAX);
}

static void
_draw(void)
{
        vdp2_scrn_scroll_x_set(SCRN_NBG1, _scroll_x);
        vdp2_scrn_scroll_y_set(SCRN_NBG1, _scroll_y);

        vdp2_scrn_reduction_x_set(SCRN_NBG1, _zoom);
        vdp2_scrn_reduction_y_set(SCRN_NBG1, _zoom);
}

static void
_vblank_in_handler(void)
{
        dma_queue_flush(DMA_QUEUE_TAG_VBLANK_IN);
}

static void
_vblank_out_handler(void)
{
        if ((vdp2_tvmd_vcount_get()) == 0) {
                _tick = (_tick & 0xFFFFFFFF) + 1;
        }

        smpc_peripheral_intback_issue();
}
