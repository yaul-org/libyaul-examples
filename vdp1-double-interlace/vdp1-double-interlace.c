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

static void _vblank_out_handler(void);

static void _vdp1_drawing_list_set(const uint8_t, struct vdp1_cmdt_list *);
static void _vdp1_drawing_env_toggle(const uint8_t);
static void _vdp2_resolution_toggle(const uint8_t);

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

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);

        uint8_t switch_env;
        switch_env = 0;
        bool switched;
        switched = true;

        fix16_t speed;
        speed = F16(0.0f);

        struct smpc_peripheral_digital digital;

        struct vdp1_cmdt_list *cmdt_list;
        cmdt_list = vdp1_cmdt_list_alloc(5);
        assert(cmdt_list != NULL);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &digital);

                if ((digital.held.button.a) != 0) {
                        switch_env ^= 1;

                        switched = true;
                }

                if ((digital.held.button.start) != 0) {
                        smpc_smc_sysres_call();
                }

                uint32_t speed_int;
                speed_int = fix16_to_int(speed);

                volatile color_rgb555_t *palette_ptr;
                palette_ptr = &_palette[0];

                palette_ptr->r = speed_int;
                palette_ptr->g = speed_int;
                palette_ptr->b = speed_int;

                MEMORY_WRITE(16, VDP2_CRAM(0x20), _palette[0].raw);

                char buffer[64];
                dbgio_buffer("[H[2Jspeed: ");
                fix16_to_str(speed, buffer, 7);
                dbgio_buffer(buffer);
                dbgio_buffer("\n");

                speed = (speed >= F16(31.0f)) ? F16(0.0f) : (speed + F16(0.5f));

                if (switched) {
                        _vdp1_drawing_list_set(switch_env, cmdt_list);
                        _vdp1_drawing_env_toggle(switch_env);
                        _vdp2_resolution_toggle(switch_env);

                        switched = false;
                }

                dbgio_flush();

                vdp1_sync_draw(cmdt_list, NULL, NULL);
                vdp_sync();
        }
}

static void
_hardware_init(void)
{
        smpc_init();
        smpc_peripheral_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_DOUBLE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB555(0, 0, 0));

        vdp2_sprite_priority_set(0, 6);
        vdp2_sprite_priority_set(1, 6);
        vdp2_sprite_priority_set(2, 6);
        vdp2_sprite_priority_set(3, 6);
        vdp2_sprite_priority_set(4, 6);
        vdp2_sprite_priority_set(5, 6);
        vdp2_sprite_priority_set(6, 6);
        vdp2_sprite_priority_set(7, 6);

        vdp_sync_vblank_out_set(_vblank_out_handler);

        vdp2_tvmd_display_set();

        cpu_intc_mask_set(0);
}

static void
_vblank_out_handler(void)
{
        smpc_peripheral_intback_issue();
}

static void
_vdp1_drawing_list_set(const uint8_t switch_env, struct vdp1_cmdt_list *cmdt_list)
{
        static const struct vdp1_cmdt_local_coord local_coord = {
                .coord = {
                        .x = 0,
                        .y = 0
                }
        };

        static struct vdp1_cmdt_system_clip_coord system_clip = {
                .coord = {
                        .x = SCREEN_WIDTH - 1,
                        .y = SCREEN_HEIGHT - 1
                }
        };

        static struct vdp1_cmdt_polygon polygon = {
                .draw_mode.raw = 0x0000,
                .draw_mode.pre_clipping_disable = true,
                .sprite_type.type_8.data.dc = 16,
                .vertex.a.x = 0,
                .vertex.a.y = SCREEN_HEIGHT - 1,

                .vertex.b.x = SCREEN_WIDTH - 1,
                .vertex.b.y = SCREEN_HEIGHT - 1,

                .vertex.c.x = SCREEN_WIDTH - 1,
                .vertex.c.y = 0,

                .vertex.d.x = 0,
                .vertex.d.y = 0
        };

        switch (switch_env & 0x01) {
        case 0:
                system_clip.coord.x = SCREEN_WIDTH - 1;
                system_clip.coord.y = SCREEN_HEIGHT - 1;

                polygon.sprite_type.type_8.data.dc = 16;
                polygon.vertex.a.y = SCREEN_HEIGHT - 1;
                polygon.vertex.b.x = SCREEN_WIDTH - 1;
                polygon.vertex.b.y = SCREEN_HEIGHT - 1;
                polygon.vertex.c.x = SCREEN_WIDTH - 1;
                break;
        case 1:
                system_clip.coord.x = (SCREEN_WIDTH / 2) - 1;
                system_clip.coord.y = (SCREEN_HEIGHT / 2) - 1;

                polygon.sprite_type.type_0.data.dc = 16;
                polygon.vertex.a.y = (SCREEN_HEIGHT / 2) - 1;
                polygon.vertex.b.x = (SCREEN_WIDTH / 2) - 1;
                polygon.vertex.b.y = (SCREEN_HEIGHT / 2) - 1;
                polygon.vertex.c.x = (SCREEN_WIDTH / 2) - 1;
                break;
        }

        assert(cmdt_list != NULL);

        vdp1_cmdt_list_reset(cmdt_list);
        vdp1_cmdt_system_clip_coord_add(cmdt_list, &system_clip);
        vdp1_cmdt_local_coord_add(cmdt_list, &local_coord);
        vdp1_cmdt_polygon_add(cmdt_list, &polygon);
        vdp1_cmdt_end(cmdt_list);
}

static void
_vdp1_drawing_env_toggle(const uint8_t switch_env)
{
        static struct vdp1_env vdp1_env = {
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

        switch (switch_env & 0x01) {
        case 0:
                vdp1_env.bpp = VDP1_ENV_BPP_8;
                vdp1_env.sprite_type = 0x8;
                vdp1_env.color_mode = VDP1_ENV_COLOR_MODE_PALETTE;

                vdp1_env.erase_points[1].x = SCREEN_WIDTH - 1;
                vdp1_env.erase_points[1].y = SCREEN_HEIGHT - 1;
                break;
        case 1:
                vdp1_env.bpp = VDP1_ENV_BPP_16;
                vdp1_env.sprite_type = 0x0;
                vdp1_env.color_mode = VDP1_ENV_COLOR_MODE_RGB_PALETTE;

                vdp1_env.erase_points[1].x = (SCREEN_WIDTH / 2) - 1;
                vdp1_env.erase_points[1].y = (SCREEN_HEIGHT / 2) - 1;
                break;
        }

        vdp1_env_set(&vdp1_env);
}

static void
_vdp2_resolution_toggle(const uint8_t switch_env)
{
        switch (switch_env) {
        case 0:
                vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_DOUBLE, VDP2_TVMD_HORZ_NORMAL_A,
                    VDP2_TVMD_VERT_240);
                break;
        case 1:
                vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
                    VDP2_TVMD_VERT_240);
                break;
        }
}
