/*
 * Copyright (c) 2012-2017 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   224

static void _hardware_init(void);

static void _setup_drawing_env(struct vdp1_cmdt_list *, bool);
static void _setup_clear_fb(struct vdp1_cmdt_list *, const color_rgb555_t, bool);

void
main(void)
{
        _hardware_init();

        struct vdp1_cmdt_list *cmdt_lists[2];

        cmdt_lists[0] = vdp1_cmdt_list_alloc(5);

        _setup_drawing_env(cmdt_lists[0], false);
        _setup_clear_fb(cmdt_lists[0], COLOR_RGB555(31, 0, 0), true);

        vdp1_sync_draw(cmdt_lists[0]);

        /* Process another list while drawing */

        cmdt_lists[1] = vdp1_cmdt_list_alloc(2);

        struct vdp1_cmdt_polygon polygon;

        polygon.cp_mode.raw = 0x0000;
        polygon.cp_color = COLOR_RGB555(0, 31, 0);
        polygon.cp_vertex.a.x = 0;
        polygon.cp_vertex.a.y = (SCREEN_HEIGHT / 2) - 1;

        polygon.cp_vertex.b.x = (SCREEN_WIDTH / 2) - 1;
        polygon.cp_vertex.b.y = (SCREEN_HEIGHT / 2) - 1;

        polygon.cp_vertex.c.x = (SCREEN_WIDTH / 2) - 1;
        polygon.cp_vertex.c.y = 0;

        polygon.cp_vertex.d.x = 0;
        polygon.cp_vertex.d.y = 0;

        vdp1_cmdt_polygon_draw(cmdt_lists[1], &polygon);
        vdp1_cmdt_end(cmdt_lists[1]);

        vdp1_sync_draw(cmdt_lists[1]);

        vdp_sync(0);

        vdp1_cmdt_list_free(cmdt_lists[0]);
        vdp1_cmdt_list_free(cmdt_lists[1]);

        while (true) {
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        vdp2_sprite_priority_set(0, 6);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_setup_drawing_env(struct vdp1_cmdt_list *cmdt_list, bool end)
{
        struct vdp1_cmdt_local_coord local_coord = {
                .lc_coord = {
                        .x = 0,
                        .y = 0
                }
        };

        struct vdp1_cmdt_system_clip_coord system_clip = {
                .scc_coord = {
                        .x = SCREEN_WIDTH - 1,
                        .y = SCREEN_HEIGHT - 1
                }
        };

        struct vdp1_cmdt_user_clip_coord user_clip = {
                .ucc_coords = {
                        {
                                .x = 0,
                                .y = 0
                        },
                        {
                                .x = SCREEN_WIDTH - 1,
                                .y = SCREEN_HEIGHT - 1
                        }
                }
        };

        vdp1_cmdt_system_clip_coord_set(cmdt_list, &system_clip);
        vdp1_cmdt_user_clip_coord_set(cmdt_list, &user_clip);
        vdp1_cmdt_local_coord_set(cmdt_list, &local_coord);

        if (end) {
                vdp1_cmdt_end(cmdt_list);
        }
}

static void
_setup_clear_fb(struct vdp1_cmdt_list *cmdt_list, const color_rgb555_t color, bool end)
{
        struct vdp1_cmdt_polygon polygon;

        polygon.cp_mode.raw = 0x0000;
        polygon.cp_color = color;
        polygon.cp_vertex.a.x = 0;
        polygon.cp_vertex.a.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.b.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.b.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.c.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.c.y = 0;

        polygon.cp_vertex.d.x = 0;
        polygon.cp_vertex.d.y = 0;

        vdp1_cmdt_polygon_draw(cmdt_list, &polygon);

        if (end) {
                vdp1_cmdt_end(cmdt_list);
        }
}
