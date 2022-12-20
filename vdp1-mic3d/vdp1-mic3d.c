/*
 * Copyright (c) 2006-2018
 * See LICENSE for details.
 *
 * Mic
 * Shazz
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#include "mic3d.h"

#define ORDER_SYSTEM_CLIP_COORDS_INDEX  0
#define ORDER_CLEAR_LOCAL_COORDS_INDEX  1
#define ORDER_BUFFER_STARTING_INDEX     2
#define ORDER_DRAW_END_INDEX            49
#define ORDER_COUNT                     (ORDER_DRAW_END_INDEX + 1)

extern const mesh_t mesh_m;
extern const mesh_t mesh_i;
extern const mesh_t mesh_c;

static void _vdp1_init(void);

void
main(void)
{
        camera_t camera;

        camera.position.x = FIX16(  0.0f);
        camera.position.y = FIX16(  0.0f);
        camera.position.z = FIX16(-30.0f);

        fix16_t theta;
        theta = FIX16(0.0f);

        mic3d_init();

        while (true) {
                render_start();

                render_mesh_start(&mesh_m);
                render_mesh_rotate(theta);
                render_mesh_translate(FIX16(-8), FIX16(0), FIX16(0));
                render_mesh_transform(&camera);

                render_mesh_start(&mesh_i);
                render_mesh_rotate(theta);
                render_mesh_transform(&camera);

                render_mesh_start(&mesh_c);
                render_mesh_rotate(theta);
                render_mesh_translate(FIX16( 5), FIX16(0), FIX16(0));
                render_mesh_transform(&camera);

                theta += FIX16(1.0f / 360.0f);

                render_process();
                render(ORDER_BUFFER_STARTING_INDEX);

                vdp1_sync_render();

                vdp1_sync();
                vdp1_sync_wait();
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        _vdp1_init();

        vdp2_sprite_priority_set(0, 6);

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();
}

static void
_vdp1_init(void)
{
        const int16_vec2_t system_clip_coord =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                   SCREEN_HEIGHT - 1);

        const int16_vec2_t local_coord_center =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH / 2,
                                   SCREEN_HEIGHT / 2);

        vdp1_cmdt_t * const cmdts = (vdp1_cmdt_t *)VDP1_CMD_TABLE(0, 0);

        vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX],
            CMDT_VTX_SYSTEM_CLIP, &system_clip_coord);

        vdp1_cmdt_local_coord_set(&cmdts[ORDER_CLEAR_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_LOCAL_COORDS_INDEX],
            CMDT_VTX_LOCAL_COORD, &local_coord_center);
}
