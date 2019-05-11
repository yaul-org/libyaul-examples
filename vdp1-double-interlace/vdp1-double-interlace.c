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

        struct vdp1_cmdt_list *cmdt_list;

        cmdt_list = vdp1_cmdt_list_alloc(5);

        _setup_drawing_env(cmdt_list, false);
        _setup_clear_fb(cmdt_list, COLOR_RGB555(31, 0, 0), true);

        vdp1_sync_draw(cmdt_list);
        vdp_sync(0);

        vdp1_cmdt_list_free(cmdt_list);

        while (true) {
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        vdp2_sprite_priority_set(0, 6);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_setup_drawing_env(struct vdp1_cmdt_list *cmdt_list, bool end)
{
        struct vdp1_cmdt_local_coord local_coord = {
                .coord = {
                        .x = 0,
                        .y = 0
                }
        };

        struct vdp1_cmdt_system_clip_coord system_clip = {
                .coord = {
                        .x = SCREEN_WIDTH - 1,
                        .y = SCREEN_HEIGHT - 1
                }
        };

        struct vdp1_cmdt_user_clip_coord user_clip = {
                .coords = {
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

        vdp1_cmdt_system_clip_coord_add(cmdt_list, &system_clip);
        vdp1_cmdt_user_clip_coord_add(cmdt_list, &user_clip);
        vdp1_cmdt_local_coord_add(cmdt_list, &local_coord);

        if (end) {
                vdp1_cmdt_end(cmdt_list);
        }
}

static void
_setup_clear_fb(struct vdp1_cmdt_list *cmdt_list, const color_rgb555_t color, bool end)
{
        struct vdp1_cmdt_polygon polygon;

        polygon.draw_mode.raw = 0x0000;
        polygon.color = color;
        polygon.vertex.a.x = 0;
        polygon.vertex.a.y = SCREEN_HEIGHT - 1;

        polygon.vertex.b.x = SCREEN_WIDTH - 1;
        polygon.vertex.b.y = SCREEN_HEIGHT - 1;

        polygon.vertex.c.x = SCREEN_WIDTH - 1;
        polygon.vertex.c.y = 0;

        polygon.vertex.d.x = 0;
        polygon.vertex.d.y = 0;

        vdp1_cmdt_polygon_add(cmdt_list, &polygon);

        if (end) {
                vdp1_cmdt_end(cmdt_list);
        }
}
