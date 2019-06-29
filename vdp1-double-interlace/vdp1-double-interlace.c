/*
 * Copyright (c) 2012-2017 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   480

static void _hardware_init(void);

static void _drawing_init(void);

static struct vdp1_cmdt_list *_cmdt_list;

static color_rgb555_t _palette[16] = {
        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),

        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),

        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),

        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0),
        COLOR_RGB555_INITIALIZER(0, 0, 0)
};

void
main(void)
{
        _hardware_init();

        _drawing_init();

        while (true) {
                (void)memcpy((void *)VDP2_CRAM_ADDR(0x10), &_palette[0], sizeof(_palette));

                _palette[0].r = (_palette[0].r + 1) & 31;
                _palette[0].g = (_palette[0].g + 1) & 31;
                _palette[0].b = (_palette[0].b + 1) & 31;

                vdp1_sync_draw(_cmdt_list, NULL, NULL);
                vdp_sync(0);
        }
}

static void
_hardware_init(void)
{
        const struct vdp1_env vdp1_env = {
                .erase_color = COLOR_RGB555(0, 0, 0),
                .erase_points[0] = {
                    .x = 0,
                    .y = 0
                },
                .erase_points[1] = {
                    .x = SCREEN_WIDTH - 1,
                    .y = SCREEN_HEIGHT -1
                },
                .bpp = VDP1_ENV_BPP_8,
                .rotation = VDP1_ENV_ROTATION_0,
                .color_mode = VDP1_ENV_COLOR_MODE_PALETTE,
                .sprite_type = 0x8
        };

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_DOUBLE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB555(0, 0, 0));

        vdp2_sprite_priority_set(0, 6);

        vdp1_env_set(&vdp1_env);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_drawing_init(void)
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

        struct vdp1_cmdt_polygon polygon;

        polygon.draw_mode.raw = 0x0000;
        polygon.draw_mode.pre_clipping_disable = true;
        polygon.sprite_type.type_8.data.dc = 16;
        polygon.vertex.a.x = 0;
        polygon.vertex.a.y = SCREEN_HEIGHT - 1;

        polygon.vertex.b.x = SCREEN_WIDTH - 1;
        polygon.vertex.b.y = SCREEN_HEIGHT - 1;

        polygon.vertex.c.x = SCREEN_WIDTH - 1;
        polygon.vertex.c.y = 0;

        polygon.vertex.d.x = 0;
        polygon.vertex.d.y = 0;

        _cmdt_list = vdp1_cmdt_list_alloc(5);

        vdp1_cmdt_system_clip_coord_add(_cmdt_list, &system_clip);
        vdp1_cmdt_local_coord_add(_cmdt_list, &local_coord);
        vdp1_cmdt_polygon_add(_cmdt_list, &polygon);
        vdp1_cmdt_end(_cmdt_list);
}
