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

typedef struct aabb aabb_t;

struct aabb {
        int16_vector2_t center;
        int16_vector2_t min;
        int16_vector2_t max;
} __packed;

static struct smpc_peripheral_digital digital_pad;

static bg_t bg_nbg0;
static bg_t bg_nbg1;

static void hardware_init(void);
static void vblank_in_handler(irq_mux_handle_t *);

/* Helpers */
static void cpu_load_map(const char *, const bg_t *, uint16_t, uint16_t) __unused;
static void cpu_copy_file(const char *, void *) __unused;
static void *cpu_dup_file(const char *) __unused;

static void debug_draw_aabb(const aabb_t *, struct vdp1_cmdt_polygon *, uint16_t);

/* Used for debugging */
static char _text_buf[1024] __unused;

void
main(void)
{
        fs_init();

        hardware_init();
        /* cons_init(CONS_DRIVER_VDP2, 40, 30); */

#define TILE(x, y)      (((x) + ((y) * (384 / 16))) * 0x0100)

        vdp2_tvmd_vblank_in_wait();

        volatile uint16_t *pnd_nbg1;
        pnd_nbg1 = (volatile uint16_t *)bg_nbg1.a_pages[0];

        uint32_t y;
        for (y = 0; y < 32; y++) {
                uint32_t x;
                for (x = 0; x < 32; x++) {
                        pnd_nbg1[x + (y * 32)] =
                            SCRN_PND_CONFIG_6((uint32_t)bg_nbg1.cp_table + TILE(6, 2),
                                (uint32_t)bg_nbg1.color_palette,
                                0,      /* VF */
                                0);     /* HF */
                }
        }

        volatile uint16_t *pnd_nbg0;
        pnd_nbg0 = (volatile uint16_t *)bg_nbg0.a_pages[0];

        pnd_nbg0[0 + (32 * 0)] = SCRN_PND_CONFIG_6((uint32_t)bg_nbg0.cp_table + TILE( 7, 2),
            (uint32_t)bg_nbg0.color_palette,
            0,  /* VF */
            0); /* HF */
        pnd_nbg0[9 + (32 * 7)] = SCRN_PND_CONFIG_6((uint32_t)bg_nbg0.cp_table + TILE(21, 7),
            (uint32_t)bg_nbg0.color_palette,
            0,  /* VF */
            0); /* HF */

        while (true) {
                vdp2_tvmd_vblank_out_wait();
                /* cons_buffer("[H[2J\n"); */

                vdp1_cmdt_list_begin(1); {
                        struct vdp1_cmdt_polygon polygon;
                        aabb_t aabb;

                        aabb.center.x = 159;
                        aabb.center.y = 119;
                        aabb.min.x = -15;
                        aabb.min.y = -15;
                        aabb.max.x = 15;
                        aabb.min.y = 15;

                        debug_draw_aabb(&aabb, &polygon, 0x001F);

                        vdp1_cmdt_end();
                } vdp1_cmdt_list_end(1);

                vdp2_tvmd_vblank_in_wait();
                /* cons_flush(); */
                vdp1_cmdt_list_commit();
        }
}

static void
debug_draw_aabb(
        const aabb_t *aabb,
        struct vdp1_cmdt_polygon *polygon,
        uint16_t color)
{
        polygon->cp_color = 0x8000 | color;

        polygon->cp_vertex.a.x = aabb->center.x + aabb->min.x;
        polygon->cp_vertex.a.y = aabb->center.y + aabb->max.y;

        polygon->cp_vertex.b.x = aabb->center.x + aabb->max.x;
        polygon->cp_vertex.b.y = aabb->center.y + aabb->max.y;

        polygon->cp_vertex.c.x = aabb->center.x + aabb->max.x;
        polygon->cp_vertex.c.y = aabb->center.y + aabb->min.y;

        polygon->cp_vertex.d.x = aabb->center.x + aabb->min.x;
        polygon->cp_vertex.d.y = aabb->center.y + aabb->min.y;

        vdp1_cmdt_polygon_draw(polygon);
}

static void
hardware_init(void)
{
        /* VDP2 */
        color_rgb555_t bs_color;
        bs_color = COLOR_RGB555(0x55 >> 3, 0x28 >> 3, 0x3A >> 3);

        vdp2_init();
        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A, TVMD_VERT_240);
        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE), bs_color);
        vdp2_tvmd_display_clear();

        /* VDP1 */
        vdp1_init();

        /* SMPC */
        smpc_init();
        smpc_peripheral_init();

        uint32_t mask;
        mask = cpu_intc_mask_get();

        cpu_intc_mask_set (0x0F); {
                irq_mux_t *vblank_in;
                vblank_in = vdp2_tvmd_vblank_in_irq_get();

                irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);
        } cpu_intc_mask_set(mask);

        vdp1_cmdt_list_begin(0); {
                        struct vdp1_cmdt_local_coord local_coord;
                        local_coord.lc_coord.x = 0;
                        local_coord.lc_coord.y = 0;

                        struct vdp1_cmdt_system_clip_coord system_clip;
                        system_clip.scc_coord.x = SCREEN_WIDTH - 1;
                        system_clip.scc_coord.y = SCREEN_HEIGHT - 1;

                        struct vdp1_cmdt_user_clip_coord user_clip;
                        user_clip.ucc_coords[0].x = 0;
                        user_clip.ucc_coords[0].y = 0;
                        user_clip.ucc_coords[1].x = SCREEN_WIDTH - 1;
                        user_clip.ucc_coords[1].y = SCREEN_HEIGHT - 1;

                        vdp1_cmdt_system_clip_coord_set(&system_clip);
                        vdp1_cmdt_user_clip_coord_set(&user_clip);
                        vdp1_cmdt_local_coord_set(&local_coord);
        } vdp1_cmdt_list_end(0);

        vdp2_tvmd_display_clear();

        memset(&bg_nbg0, 0x00, sizeof(bg_nbg0));
        memset(&bg_nbg1, 0x00, sizeof(bg_nbg1));

        bg_nbg1.format.scf_scroll_screen = SCRN_NBG1;
        bg_nbg1.format.scf_cc_count = SCRN_CCC_PALETTE_256;
        bg_nbg1.format.scf_character_size = 2 * 2;
        bg_nbg1.format.scf_pnd_size = 1;
        bg_nbg1.format.scf_auxiliary_mode = 0;
        bg_nbg1.format.scf_sf_type = SCRN_SF_TYPE_NONE;
        bg_nbg1.format.scf_sf_code = SCRN_SF_CODE_A;
        bg_nbg1.format.scf_sf_mode = 0;
        bg_nbg1.format.scf_plane_size = 1 * 1;
        bg_nbg1.format.scf_cp_table = (uint32_t)VRAM_ADDR_4MBIT(0, 0x10000);
        bg_nbg1.format.scf_color_palette = (uint32_t)CRAM_MODE_1_OFFSET(0, 0, 0);
        bg_nbg1.format.scf_map.plane_a = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_nbg1.format.scf_map.plane_b = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_nbg1.format.scf_map.plane_c = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);
        bg_nbg1.format.scf_map.plane_d = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000);

        bg_nbg0.format.scf_scroll_screen = SCRN_NBG0;
        bg_nbg0.format.scf_cc_count = SCRN_CCC_PALETTE_256;
        bg_nbg0.format.scf_character_size = 2 * 2;
        bg_nbg0.format.scf_pnd_size = 1;
        bg_nbg0.format.scf_auxiliary_mode = 0;
        bg_nbg0.format.scf_sf_type = SCRN_SF_TYPE_PRIORITY;
        bg_nbg0.format.scf_sf_code = SCRN_SF_CODE_A;
        bg_nbg0.format.scf_sf_mode = 2;
        bg_nbg0.format.scf_plane_size = 1 * 1;
        bg_nbg0.format.scf_cp_table = (uint32_t)VRAM_ADDR_4MBIT(0, 0x10000);
        bg_nbg0.format.scf_color_palette = (uint32_t)CRAM_MODE_1_OFFSET(0, 0, 0);
        bg_nbg0.format.scf_map.plane_a = (uint32_t)VRAM_ADDR_4MBIT(1, 0x01000);
        bg_nbg0.format.scf_map.plane_b = (uint32_t)VRAM_ADDR_4MBIT(1, 0x01000);
        bg_nbg0.format.scf_map.plane_c = (uint32_t)VRAM_ADDR_4MBIT(1, 0x01000);
        bg_nbg0.format.scf_map.plane_d = (uint32_t)VRAM_ADDR_4MBIT(1, 0x01000);

        vdp2_scrn_cell_format_set(&bg_nbg0.format);
        vdp2_scrn_cell_format_set(&bg_nbg1.format);

        vdp2_scrn_priority_set(SCRN_NBG0, 2);
        vdp2_scrn_priority_set(SCRN_NBG1, 2);
        vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ true);
        vdp2_scrn_display_set(SCRN_NBG1, /* transparent = */ true);

        vdp2_scrn_sf_codes_set(SCRN_SF_CODE_A, SCRN_SF_CODE_0x00_0x01 | SCRN_SF_CODE_0x02_0x03 | SCRN_SF_CODE_0x04_0x05);

        vdp2_sprite_priority_set(0, 2);
        vdp2_sprite_priority_set(1, 2);
        vdp2_sprite_priority_set(2, 2);
        vdp2_sprite_priority_set(3, 2);
        vdp2_sprite_priority_set(4, 2);
        vdp2_sprite_priority_set(5, 2);
        vdp2_sprite_priority_set(6, 2);
        vdp2_sprite_priority_set(7, 2);

        bg_calculate_params(&bg_nbg1);
        bg_calculate_params(&bg_nbg0);

        bg_clear(&bg_nbg1);
        bg_clear(&bg_nbg0);

        struct vram_ctl *vram_ctl;
        vram_ctl = vdp2_vram_control_get();

        vram_ctl->vram_cycp.pt[0].t0 = VRAM_CTL_CYCP_CHPNDR_NBG2;
        vram_ctl->vram_cycp.pt[0].t1 = VRAM_CTL_CYCP_CHPNDR_NBG2;
        vram_ctl->vram_cycp.pt[0].t2 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[0].t4 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[0].t5 = VRAM_CTL_CYCP_CHPNDR_NBG0;
        vram_ctl->vram_cycp.pt[0].t6 = VRAM_CTL_CYCP_CHPNDR_NBG1;
        vram_ctl->vram_cycp.pt[0].t7 = VRAM_CTL_CYCP_CHPNDR_NBG1;

        vram_ctl->vram_cycp.pt[1].t0 = VRAM_CTL_CYCP_PNDR_NBG0;
        vram_ctl->vram_cycp.pt[1].t1 = VRAM_CTL_CYCP_PNDR_NBG1;
        vram_ctl->vram_cycp.pt[1].t2 = VRAM_CTL_CYCP_PNDR_NBG2;
        vram_ctl->vram_cycp.pt[1].t3 = VRAM_CTL_CYCP_PNDR_NBG3;
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
        vram_ctl->vram_cycp.pt[3].t2 = VRAM_CTL_CYCP_CHPNDR_NBG3;
        vram_ctl->vram_cycp.pt[3].t1 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t3 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t4 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t5 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t6 = VRAM_CTL_CYCP_NO_ACCESS;
        vram_ctl->vram_cycp.pt[3].t7 = VRAM_CTL_CYCP_NO_ACCESS;

        vdp2_vram_control_set(vram_ctl);

        cpu_copy_file("/TILESET.CEL", (void *)VRAM_ADDR_4MBIT(0, 0x10000));
        cpu_copy_file("/TILESET.PAL", (void *)CRAM_MODE_1_OFFSET(0, 0, 0));

        vdp2_tvmd_display_set();
}

static void
vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        vdp2_commit();

        smpc_peripheral_digital_port(1, &digital_pad);
        vdp2_commit();
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
