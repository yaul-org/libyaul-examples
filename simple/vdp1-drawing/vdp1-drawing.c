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

static struct vdp1_cmdt *_setup_drawing(struct vdp1_cmdt *, bool);
static struct vdp1_cmdt *_clear_fb(struct vdp1_cmdt *, const color_rgb555_t, bool);

void
main(void)
{
        _hardware_init();

        struct vdp1_cmdt *cmdts;
        cmdts = malloc(sizeof(struct vdp1_cmdt) * 8);
        assert(cmdts != NULL);

        struct vdp1_cmdt *cmdt_p;
        cmdt_p = cmdts;
        cmdt_p = _setup_drawing(cmdt_p, false);
        cmdt_p = _clear_fb(cmdt_p, COLOR_RGB555(31, 0, 0), true);

        vdp1_sync_draw(cmdts, cmdt_p - cmdts);

        /* Reset pointer */
        cmdt_p = cmdts;

        struct vdp1_cmdt_polygon polygon;

        polygon.cp_mode.raw = 0x0000;
        polygon.cp_color = 0x83C0;
        polygon.cp_vertex.a.x = 0;
        polygon.cp_vertex.a.y = (SCREEN_HEIGHT / 2) - 1;

        polygon.cp_vertex.b.x = (SCREEN_WIDTH / 2) - 1;
        polygon.cp_vertex.b.y = (SCREEN_HEIGHT / 2) - 1;

        polygon.cp_vertex.c.x = (SCREEN_WIDTH / 2) - 1;
        polygon.cp_vertex.c.y = 0;

        polygon.cp_vertex.d.x = 0;
        polygon.cp_vertex.d.y = 0;

        cmdt_p = vdp1_cmdt_polygon_draw(cmdt_p, &polygon);
        cmdt_p = vdp1_cmdt_end(cmdt_p);

        vdp1_sync_draw(cmdts, cmdt_p - cmdts);

        dbgio_flush();
        vdp2_sync_commit();
        vdp_sync(0);

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

        vdp2_sprite_type_set(0);
        vdp2_sprite_priority_set(0, 6);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static struct vdp1_cmdt *
_setup_drawing(struct vdp1_cmdt *cmdt_p, bool end)
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

        cmdt_p = vdp1_cmdt_system_clip_coord_set(cmdt_p, &system_clip);
        cmdt_p = vdp1_cmdt_user_clip_coord_set(cmdt_p, &user_clip);
        cmdt_p = vdp1_cmdt_local_coord_set(cmdt_p, &local_coord);

        if (end) {
                cmdt_p = vdp1_cmdt_end(cmdt_p);
        }

        return cmdt_p;
}

static struct vdp1_cmdt *
_clear_fb(struct vdp1_cmdt *cmdt_p, const color_rgb555_t color, bool end)
{
        struct vdp1_cmdt_polygon polygon;

        polygon.cp_mode.raw = 0x0000;
        polygon.cp_color = (uint16_t)color.raw | 0x8000;
        polygon.cp_vertex.a.x = 0;
        polygon.cp_vertex.a.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.b.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.b.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.c.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.c.y = 0;

        polygon.cp_vertex.d.x = 0;
        polygon.cp_vertex.d.y = 0;

        cmdt_p = vdp1_cmdt_polygon_draw(cmdt_p, &polygon);

        if (end) {
                cmdt_p = vdp1_cmdt_end(cmdt_p);
        }

        return cmdt_p;
}
