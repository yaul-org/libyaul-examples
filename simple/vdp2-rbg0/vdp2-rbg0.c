/*-
 * Copyright (c) 2006-2018 Israel Jacquez <mrkotfw@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <yaul.h>

#include <sys/queue.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "fs.h"
#include "bg.h"

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

static struct smpc_peripheral_digital digital_pad;

static bg_t bg_rbg0 __unused;

static void hardware_init(void);
static void vblank_in_handler(irq_mux_handle_t *);

/* Helpers */
static void cpu_load_map(const char *, const bg_t *, uint16_t, uint16_t) __unused;
static void cpu_copy_file(const char *, void *) __unused;
static void *cpu_dup_file(const char *) __unused;

struct vdp2_scrn_rotation_table {
        /* Screen start coordinates */
        uint32_t xst;
        uint32_t yst;
        uint32_t zst;

        /* Screen vertical coordinate increments (per each line) */
        uint32_t delta_xst;
        uint32_t delta_yst;

        /* Screen horizontal coordinate increments (per each dot) */
        uint32_t delta_x;
        uint32_t delta_y;

        /* Rotation matrix
         *
         * A B C
         * D E F
         * G H I
         */
        union {
                uint32_t raw[2][3];     /* XXX: Needs to be tested */

                struct {
                        uint32_t a;
                        uint32_t b;
                        uint32_t c;
                        uint32_t d;
                        uint32_t e;
                        uint32_t f;
                } param __packed;

                uint32_t params[6];     /* Parameters A, B, C, D, E, F */
        } matrix;

        /* View point coordinates */
        uint16_t px;
        uint16_t py;
        uint16_t pz;

        unsigned int :16;

        /* Center coordinates */
        uint16_t cx;
        uint16_t cy;
        uint16_t cz;

        unsigned int :16;

        /* Amount of horizontal shifting */
        uint32_t mx;
        uint32_t my;

        /* Scaling coefficients */
        uint32_t kx;            /* Expansion/reduction coefficient X */
        uint32_t ky;            /* Expansion/reduction coefficient Y */

        /* Coefficient table address */
        uint32_t kast;          /* Coefficient table start address */
        uint32_t delta_kast;    /* Addr. increment coeff. table (per line) */
        uint32_t delta_kax;     /* Addr. increment coeff. table (per dot) */
} __packed;

void
main(void)
{
        fs_init();

        hardware_init();

        struct vdp2_scrn_rotation_table *rot_tbl __unused;
        rot_tbl = (struct vdp2_scrn_rotation_table *)VRAM_ADDR_4MBIT(2, 0x00000);

        while (true) {
                vdp2_tvmd_vblank_out_wait();
                vdp2_tvmd_vblank_in_wait();
        }
}

static void
hardware_init(void)
{
        /* VDP2 */
        color_rgb555_t bs_color;
        bs_color = COLOR_RGB555(0x2F >> 3, 0x28 >> 3, 0x3A >> 3);

        vdp2_init();

        vdp2_sprite_type_set(0);
        vdp2_sprite_priority_set(0, 0);

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A, TVMD_VERT_240);
        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE), bs_color);
        vdp2_tvmd_display_clear();

        smpc_init();
        smpc_peripheral_init();

        cpu_intc_mask_set (15); {
                irq_mux_t *vblank_in;
                vblank_in = vdp2_tvmd_vblank_in_irq_get();

                irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);
        } cpu_intc_mask_set(0);

        vdp2_tvmd_display_clear();

        memset(&bg_rbg0, 0x00, sizeof(bg_rbg0));

        bg_rbg0.format.scf_scroll_screen = SCRN_RBG0;
        bg_rbg0.format.scf_cc_count = SCRN_CCC_PALETTE_256;
        bg_rbg0.format.scf_character_size = 2 * 2;
        bg_rbg0.format.scf_pnd_size = 1; /* 1-word */
        bg_rbg0.format.scf_auxiliary_mode = 0;
        bg_rbg0.format.scf_plane_size = 2 * 2;
        bg_rbg0.format.scf_cp_table = (uint32_t)VRAM_ADDR_4MBIT(0, 0x10000);
        bg_rbg0.format.scf_color_palette = (uint32_t)CRAM_MODE_1_OFFSET(0, 0, 0);
        bg_rbg0.format.scf_map.plane_a = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_b = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_c = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_d = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_d = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_e = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_f = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_g = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_h = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_i = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_j = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_k = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_l = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_m = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_n = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_o = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_rbg0.format.scf_map.plane_p = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);

        vdp2_scrn_cell_format_set(&bg_rbg0.format);

        vdp2_scrn_priority_set(SCRN_RBG0, 3);
        vdp2_scrn_display_set(SCRN_RBG0, /* transparent = */ true);

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        bg_calculate_params(&bg_rbg0);

        bg_clear(&bg_rbg0);

        struct vram_ctl *vram_ctl;
        vram_ctl = vdp2_vram_control_get();

        vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_NO_ACCESS;
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
        vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vdp2_vram_control_set(vram_ctl);

        struct vdp2_scrn_rotation_table *rot_tbl __unused;
        rot_tbl = (struct vdp2_scrn_rotation_table *)VRAM_ADDR_4MBIT(2, 0x00000);

        memset(rot_tbl, 0x00, sizeof(*rot_tbl));

        rot_tbl->xst = 0;
        rot_tbl->yst = 0;
        rot_tbl->zst = 0;

        rot_tbl->delta_xst = 0x00000000;
        rot_tbl->delta_yst = 0x00010000;

        rot_tbl->delta_x = 0x00010000;
        rot_tbl->delta_y = 0x00000000;

        rot_tbl->matrix.param.a = 0x00010000;
        rot_tbl->matrix.param.b = 0x00000000;
        rot_tbl->matrix.param.c = 0x00000000;
        rot_tbl->matrix.param.d = 0x00000000;
        rot_tbl->matrix.param.e = 0x00010000;
        rot_tbl->matrix.param.f = 0x00000000;

        rot_tbl->px = 0;
        rot_tbl->py = 0;
        rot_tbl->pz = 0;

        rot_tbl->cx = 0;
        rot_tbl->cy = 0;
        rot_tbl->cz = 0;

        rot_tbl->mx = 0;
        rot_tbl->my = 0;

        rot_tbl->kx = 0x00010000;
        rot_tbl->ky = 0x00010000;

        rot_tbl->kast = 0;
        rot_tbl->delta_kast = 0;
        rot_tbl->delta_kax = 0;


        cpu_copy_file("/TILESET.CEL", (void *)VRAM_ADDR_4MBIT(0, 0x10000));
        cpu_copy_file("/TILESET.PAL", (void *)CRAM_MODE_0_OFFSET(0, 0, 0));
        cpu_load_map("/LEVEL1.MAP", &bg_rbg0, 32, 32);

        vdp2_tvmd_display_set();
}

static void
vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        smpc_peripheral_digital_port(1, &digital_pad);
        vdp2_commit();
        
        /* Remove this hack. We have to wait until SCU DMA is done
         * copying the buffered VDP2 registers to memory. */
        scu_dma_level0_wait();

        MEMORY_WRITE(16, VDP2(RAMCTL), 0x030B);
        MEMORY_WRITE(16, VDP2(RPTAU), 0x0002);
        MEMORY_WRITE(16, VDP2(RPTAL), ((uint32_t)VRAM_ADDR_4MBIT(2, 0x00000)) & 0xFFFE);
}

static void
cpu_load_map(
        const char *map_path,
        const bg_t *bg,
        uint16_t map_width,
        uint16_t map_height)
{
        uint16_t *map;
        map = cpu_dup_file(map_path);

        uint32_t page_x;
        uint32_t page_y;
        for (page_y = 0; page_y < map_height; page_y++) {
                for (page_x = 0; page_x < map_width; page_x++) {
                        uint16_t page_idx;
                        page_idx = page_x + (bg->page_width * page_y);

                        uint16_t raw_pnd;
                        raw_pnd = map[page_x + (map_width * page_y)];

                        bg->a_pages[0][page_idx] = raw_pnd;
                }
        }

        free(map);
}

static void
cpu_copy_file(const char *path, void *dst)
{
        void *fh;

        fh = fs_open(path);
        assert(fh != NULL);

        fs_read_whole(fh, dst);

        fs_close(fh);
}

static void *
cpu_dup_file(const char *path)
{
        void *fh;
        fh = fs_open(path);
        assert(fh != NULL);

        size_t file_size;
        file_size = fs_size(fh);

        uint16_t *buffer;
        buffer = (uint16_t *)malloc(file_size);

        fs_read_whole(fh, buffer);
        fs_close(fh);

        return buffer;
}
