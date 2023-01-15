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
#define ORDER_LOCAL_COORDS_INDEX        1
#define ORDER_SUBR_INDEX                1
#define ORDER_DRAW_END_INDEX            2
#define ORDER_INDEX                     3

extern const mesh_t mesh_m;
extern const mesh_t mesh_i;
extern const mesh_t mesh_c;
extern const mesh_t mesh_cube;
extern const mesh_t mesh_torus;
extern const mesh_t mesh_torus2;
extern const mesh_t mesh_torus3;

extern const picture_t picture_mika;
extern const picture_t picture_tails;
extern const picture_t picture_baku;

extern const palette_t palette_baku;

static texture_t _textures[8];

static void _vdp1_init(void);

static size_t _texture_load(texture_t *textures, uint32_t slot, const picture_t *picture, vdp1_vram_t texture_base);
static void _palette_load(uint16_t bank_256, uint16_t bank_16, const palette_t *palette);

vdp1_gouraud_table_t _pool_shading_tables[CMDT_COUNT] __aligned(16);

void
main(void)
{
        for (uint32_t i = 0; i < CMDT_COUNT; i++) {
                _pool_shading_tables[i].colors[0] = RGB1555(1, i & 31,     16,     16);
                _pool_shading_tables[i].colors[1] = RGB1555(1,     16, i & 31,     16);
                _pool_shading_tables[i].colors[2] = RGB1555(1,     16,     16, i & 31);
                _pool_shading_tables[i].colors[3] = RGB1555(1, i & 31, i & 31, i & 31);
        }

        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        vdp1_vram_partitions_t vdp1_vram_partitions;

        vdp1_vram_partitions_get(&vdp1_vram_partitions);

        mic3d_init();

        tlist_set(_textures, 8);

        light_set(_pool_shading_tables,
            CMDT_COUNT,
            (vdp1_vram_t)vdp1_vram_partitions.gouraud_base);

        vdp1_vram_t texture_base;
        texture_base = (vdp1_vram_t)vdp1_vram_partitions.texture_base;

        texture_base += _texture_load(_textures, 0, &picture_mika, texture_base);
        texture_base += _texture_load(_textures, 1, &picture_tails, texture_base);
        texture_base += _texture_load(_textures, 2, &picture_baku, texture_base);

        _palette_load(0, 0, &palette_baku);

        camera_t camera __unused;

        camera.position.x = FIX16(  0.0f);
        camera.position.y = FIX16(  0.0f);
        camera.position.z = FIX16(-30.0f);

        camera.target.x = FIX16_ZERO;
        camera.target.y = FIX16_ZERO;
        camera.target.z = FIX16_ZERO;

        camera.up.x =  FIX16_ZERO;
        camera.up.y = -FIX16_ONE;

        camera_lookat(&camera);

        angle_t theta;
        theta = DEG2ANGLE(0.0f);

        while (true) {
                dbgio_puts("[H[2J");
                render_start();

                /* render_mesh_start(&mesh_cube); */
                /* render_disable(RENDER_FLAGS_LIGHTING); */
                /* matrix_push(); */
                /* /\* matrix_x_rotate(theta); *\/ */
                /* matrix_y_rotate(theta); */
                /* /\* matrix_z_rotate(theta); *\/ */
                /* matrix_z_translate(FIX16(30)); */
                /* render_mesh_transform(); */
                /* matrix_pop(); */

                render_enable(RENDER_FLAGS_LIGHTING);
                matrix_push();
                matrix_x_translate(FIX16(-15));
                matrix_x_rotate(theta);
                matrix_y_rotate(theta);
                matrix_z_rotate(theta);
                matrix_x_translate(FIX16(-20));
                matrix_z_translate(FIX16(30));
                render_mesh_transform(&mesh_torus);
                matrix_pop();

                render_disable(RENDER_FLAGS_LIGHTING);
                matrix_push();
                matrix_y_rotate(theta);
                matrix_x_rotate(theta);
                matrix_z_rotate(theta);
                matrix_z_translate(FIX16(30));
                render_mesh_transform(&mesh_torus2);
                matrix_pop();

                render_enable(RENDER_FLAGS_LIGHTING);
                matrix_push();
                matrix_x_translate(FIX16(15));
                matrix_z_rotate(theta);
                matrix_y_rotate(theta);
                matrix_x_rotate(theta);
                matrix_x_translate(FIX16(20));
                matrix_z_translate(FIX16(30));
                render_mesh_transform(&mesh_torus3);
                matrix_pop();

                theta += DEG2ANGLE(5.0f);

                render(ORDER_SUBR_INDEX, ORDER_INDEX);

                vdp1_sync_render();

                vdp1_sync();
                vdp1_sync_wait();

                dbgio_flush();
                vdp2_sync();
                vdp2_sync_wait();
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_B,
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
        vdp1_cmdt_vtx_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX],
            system_clip_coord);

        vdp1_cmdt_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_vtx_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX],
            local_coord_center);

        vdp1_cmdt_end_set(&cmdts[ORDER_DRAW_END_INDEX]);

        vdp1_sync_interval_set(2);

        vdp1_env_default_set();
}

static size_t
_texture_load(texture_t *textures, uint32_t slot, const picture_t *picture, vdp1_vram_t texture_base)
{
        texture_t * const texture = &textures[slot];

        texture->size       = TEXTURE_SIZE(picture->dim.x, picture->dim.y);
        texture->vram_index = TEXTURE_VRAM_INDEX(texture_base);

        scu_dma_transfer(0, (void *)texture_base, picture->data, picture->data_size);
        scu_dma_transfer_wait(0);

        return picture->data_size;
}

static void
_palette_load(uint16_t bank_256, uint16_t bank_16, const palette_t *palette)
{
        scu_dma_transfer(0, (void *)VDP2_CRAM_MODE_0_OFFSET(bank_256, bank_16, 0), palette->data, palette->data_size);
        scu_dma_transfer_wait(0);
}
