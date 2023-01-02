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

#define PRIMITIVE_WIDTH           32
#define PRIMITIVE_HEIGHT          32
#define PRIMITIVE_HALF_WIDTH      (PRIMITIVE_WIDTH / 2)
#define PRIMITIVE_HALF_HEIGHT     (PRIMITIVE_HEIGHT / 2)

#define PRIMITIVE_0_COLOR RGB1555(1, 31,  0, 31)
#define PRIMITIVE_1_COLOR RGB1555(1, 31, 31,  0)

#define ORDER_SYSTEM_CLIP_COORDS_INDEX  0
#define ORDER_LOCAL_COORDS_INDEX        1
#define ORDER_POLYGON_INDEX             2
#define ORDER_DRAW_END_INDEX            4
#define ORDER_COUNT                     (ORDER_DRAW_END_INDEX + 1)

static smpc_peripheral_digital_t _digital;

static vdp1_cmdt_list_t *_cmdt_list = NULL;
static vdp1_vram_partitions_t _vdp1_vram_partitions;

static void _cmdt_list_init(void);
static void _primitives_init(void);

static void _primitive_move(vdp1_cmdt_t *cmdt_polygon, int16_t x, int16_t y);

static void _vblank_out_handler(void *work);

int
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2);
        dbgio_dev_font_load();

        _cmdt_list_init();
        _primitives_init();

        int16_vec2_t a = INT16_VEC2_INITIALIZER( 0,  0);
        int16_vec2_t b = INT16_VEC2_INITIALIZER(16, 16);

        dbgio_printf("Use diagonals to move 1st primitive\n"
                     "Hold B & use diagonals for 2nd primitive");
        dbgio_flush();
        vdp2_sync();

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                const uint16_t pressed_state = _digital.pressed.raw;

                vdp1_cmdt_t *cmdt_polygon;
                cmdt_polygon = &_cmdt_list->cmdts[ORDER_POLYGON_INDEX];

                int16_vec2_t *p;
                p = &a;

                if ((pressed_state & PERIPHERAL_DIGITAL_B) != 0x0000) {
                        cmdt_polygon = &_cmdt_list->cmdts[ORDER_POLYGON_INDEX + 1];
                        p = &b;
                }

                if ((pressed_state & PERIPHERAL_DIGITAL_LEFT) != 0) {
                        p->x--;
                }

                if ((pressed_state & PERIPHERAL_DIGITAL_RIGHT) != 0) {
                        p->x++;
                }

                if ((pressed_state & PERIPHERAL_DIGITAL_UP) != 0) {
                        p->y--;
                }

                if ((pressed_state & PERIPHERAL_DIGITAL_DOWN) != 0) {
                        p->y++;
                }

                _primitive_move(cmdt_polygon, p->x, p->y);

                vdp1_sync_cmdt_list_put(_cmdt_list, 0);
                vdp1_sync_render();
                vdp1_sync();
                vdp1_sync_wait();
        }
}

void
user_init(void)
{
        smpc_peripheral_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);
        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));
        vdp2_sprite_priority_set(0, 6);

        vdp1_env_t env;
        vdp1_env_default_init(&env);

        env.erase_color = RGB1555(1, 0, 3, 15);

        vdp1_env_set(&env);

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp2_tvmd_display_set();

        vdp1_vram_partitions_get(&_vdp1_vram_partitions);
}

static void
_cmdt_list_init(void)
{
        static const int16_vec2_t system_clip_coord =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                   SCREEN_HEIGHT - 1);

        _cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);

        (void)memset(&_cmdt_list->cmdts[0], 0x00,
            sizeof(vdp1_cmdt_t) * ORDER_COUNT);

        _cmdt_list->count = ORDER_COUNT;

        vdp1_cmdt_t *cmdts;
        cmdts = &_cmdt_list->cmdts[0];

        vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_vtx_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX],
            system_clip_coord);

        vdp1_cmdt_end_set(&cmdts[ORDER_DRAW_END_INDEX]);
}

static void
_primitives_init(void)
{
        static const int16_vec2_t local_coord_center =
            INT16_VEC2_INITIALIZER((SCREEN_WIDTH / 2) - PRIMITIVE_HALF_WIDTH - 1,
                                   (SCREEN_HEIGHT / 2) - PRIMITIVE_HALF_HEIGHT - 1);

        vdp1_cmdt_t *cmdt_local_coords;
        cmdt_local_coords = &_cmdt_list->cmdts[ORDER_LOCAL_COORDS_INDEX];

        vdp1_cmdt_local_coord_set(cmdt_local_coords);
        vdp1_cmdt_vtx_local_coord_set(cmdt_local_coords,
            local_coord_center);

        vdp1_cmdt_t *cmdt_polygon;

        const vdp1_cmdt_draw_mode_t polygon_draw_mode = {
                .mesh_enable = true
        };

        cmdt_polygon = &_cmdt_list->cmdts[ORDER_POLYGON_INDEX];
        vdp1_cmdt_polygon_set(cmdt_polygon);
        vdp1_cmdt_color_set(cmdt_polygon, PRIMITIVE_0_COLOR);
        vdp1_cmdt_draw_mode_set(cmdt_polygon, polygon_draw_mode);
        _primitive_move(cmdt_polygon, 0, 0);

        cmdt_polygon = &_cmdt_list->cmdts[ORDER_POLYGON_INDEX + 1];
        vdp1_cmdt_polygon_set(cmdt_polygon);
        vdp1_cmdt_color_set(cmdt_polygon, PRIMITIVE_1_COLOR);
        vdp1_cmdt_draw_mode_set(cmdt_polygon, polygon_draw_mode);
        _primitive_move(cmdt_polygon, 16, 16);
}

static void
_primitive_move(vdp1_cmdt_t *cmdt_polygon, int16_t x, int16_t y)
{
        cmdt_polygon->cmd_xa = x;
        cmdt_polygon->cmd_ya = y + (PRIMITIVE_HEIGHT - 1);

        cmdt_polygon->cmd_xb = x + (PRIMITIVE_WIDTH - 1);
        cmdt_polygon->cmd_yb = y + (PRIMITIVE_HEIGHT - 1);

        cmdt_polygon->cmd_xc = x + (PRIMITIVE_WIDTH - 1);
        cmdt_polygon->cmd_yc = y;

        cmdt_polygon->cmd_xd = x;
        cmdt_polygon->cmd_yd = y;
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}
