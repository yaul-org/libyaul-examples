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

#include <mic3d.h>

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

static size_t _texture_load(texture_t *textures, uint32_t slot, const picture_t *picture, vdp1_vram_t texture_base);
static void _palette_load(uint16_t bank_256, uint16_t bank_16, const palette_t *palette);

static vdp1_gouraud_table_t _pool_shading_tables[CONFIG_MIC3D_CMDT_COUNT] __aligned(16);
static vdp1_gouraud_table_t _pool_shading_tables2[512] __aligned(16);

static workarea_mic3d_depth_values_t _pool_depth_values;
static workarea_mic3d_z_values_t _pool_z_values;
static workarea_mic3d_screen_points_t _pool_screen_points;
static workarea_mic3d_sort_singles_t _pool_sort_singles;
static workarea_mic3d_cmdts_t _pool_cmdts;
static workarea_mic3d_render_matrices_t _pool_render_matrices;
static workarea_mic3d_light_matrices_t _pool_light_matrices;
static workarea_mic3d_colors_t _pool_colors;
static workarea_mic3d_work_t _pool_work;

static workarea_mic3d_t _workarea = {
    &_pool_depth_values,   &_pool_z_values, &_pool_screen_points,
    &_pool_sort_singles,   &_pool_cmdts,    &_pool_render_matrices,
    &_pool_light_matrices, &_pool_colors,   &_pool_work
};

static sort_list_t _sort_list[512] __aligned(4);

int
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_MEDNAFEN_DEBUG);

        vdp1_vram_partitions_t vdp1_vram_partitions;

        vdp1_vram_partitions_get(&vdp1_vram_partitions);

        mic3d_init(&_workarea);
        render_sort_depth_set(_sort_list, 512);

        tlist_set(_textures, 8);

        light_gst_set(_pool_shading_tables,
            CONFIG_MIC3D_CMDT_COUNT,
            (vdp1_vram_t)(vdp1_vram_partitions.gouraud_base + 512));

        vdp1_vram_t texture_base;
        texture_base = (vdp1_vram_t)vdp1_vram_partitions.texture_base;

        texture_base += _texture_load(_textures, 0, &picture_mika, texture_base);
        texture_base += _texture_load(_textures, 1, &picture_tails, texture_base);
        texture_base += _texture_load(_textures, 2, &picture_baku, texture_base);

        _palette_load(0, 0, &palette_baku);

        camera_t camera;

        camera.position.x = FIX16( 0.0);
        camera.position.y = FIX16( 0.0);
        camera.position.z = FIX16(10.0);

        camera.target.x = FIX16(0.0);
        camera.target.y = FIX16(0.0);
        camera.target.z = FIX16(0.0);

        camera.up.x = FIX16(0.0);
        camera.up.y = FIX16(1.0);
        camera.up.z = FIX16(0.0);

        camera_lookat(&camera);

        angle_t theta __unused;
        theta = DEG2ANGLE(0.0);

        for (uint32_t i = 0; i < 512; i++) {
                const rgb1555_t color = RGB1555(1,
                                                fix16_int32_to(fix16_int32_from(i * 31) / 512U),
                                                fix16_int32_to(fix16_int32_from(i * 31) / 512U),
                                                fix16_int32_to(fix16_int32_from(i * 31) / 512U));

                _pool_shading_tables2[i].colors[0] = color;
                _pool_shading_tables2[i].colors[1] = color;
                _pool_shading_tables2[i].colors[2] = color;
                _pool_shading_tables2[i].colors[3] = color;
        }

        gst_set((vdp1_vram_t)vdp1_vram_partitions.gouraud_base);
        gst_put(_pool_shading_tables2, 512);
        gst_unset();

        fix16_mat43_t world[2];

        fix16_mat33_identity(&world[0].rotation);
        fix16_mat33_identity(&world[1].rotation);

        world[0].translation.x = FIX16(   0.0);
        world[0].translation.y = FIX16(   0.0);
        world[0].translation.z = FIX16(-100.0);

        world[1].translation.x = FIX16(  50.0);
        world[1].translation.y = FIX16(   0.0);
        world[1].translation.z = FIX16(-100.0);

        /* Set up a command table for insertion */
        vdp1_cmdt_t cmdt_polygon;
        vdp1_cmdt_polygon_set(&cmdt_polygon);
        vdp1_cmdt_draw_mode_t polygon_draw_mode;
        polygon_draw_mode.raw = 0x0000;
        vdp1_cmdt_draw_mode_set(&cmdt_polygon, polygon_draw_mode);

        while (true) {
                dbgio_puts("[H[2J");

                /* Call this before rendering */
                render_start();

                fix16_mat43_t result;
                /* Must zero out since each XYZ rotation functions do not write
                 * to all the elements in the 3x3 matrix */
                fix16_mat33_zero(&result.rotation);

                render_enable(RENDER_FLAGS_LIGHTING);
                /* Rotate around the origin first by Z, then Y, then X, ignoring translation */
                fix16_mat33_z_rotate(&world[0].rotation, theta, &result.rotation);
                fix16_mat33_y_rotate(&result.rotation, theta, &result.rotation);
                fix16_mat33_x_rotate(&result.rotation, theta, &result.rotation);
                /* Translate */
                fix16_vec3_dup(&world[0].translation, &result.translation);
                render_mesh_xform(&mesh_torus, &result);

                render_disable(RENDER_FLAGS_LIGHTING);
                /* Take the previous matrix and just modify the translation */
                fix16_vec3_dup(&world[1].translation, &result.translation);
                render_mesh_xform(&mesh_cube, &result);

                cmdt_polygon.cmd_vertices[0].x = -10 + 20;
                cmdt_polygon.cmd_vertices[0].y = -10 + 20;
                cmdt_polygon.cmd_vertices[1].x =  10 + 20;
                cmdt_polygon.cmd_vertices[1].y = -10 + 20;
                cmdt_polygon.cmd_vertices[2].x =  10 + 20;
                cmdt_polygon.cmd_vertices[2].y =  10 + 20;
                cmdt_polygon.cmd_vertices[3].x = -10 + 20;
                cmdt_polygon.cmd_vertices[3].y =  10 + 20;
                cmdt_polygon.cmd_colr          = 0x801F;
                /* Call this before render_end() */
                render_cmdt_insert(&cmdt_polygon, FIX16( -20.0));

                cmdt_polygon.cmd_vertices[0].x = -50;
                cmdt_polygon.cmd_vertices[0].y = -50;
                cmdt_polygon.cmd_vertices[1].x =  50;
                cmdt_polygon.cmd_vertices[1].y = -50;
                cmdt_polygon.cmd_vertices[2].x =  50;
                cmdt_polygon.cmd_vertices[2].y =  50;
                cmdt_polygon.cmd_vertices[3].x = -50;
                cmdt_polygon.cmd_vertices[3].y =  50;
                cmdt_polygon.cmd_colr          = 0x83C0;
                /* Call this before render_end() */
                render_cmdt_insert(&cmdt_polygon, FIX16(-150.0));

                theta += DEG2ANGLE(2.5);

                /* End of rendering */
                render_end();

                vdp1_sync_render();
                vdp1_sync();
                vdp1_sync_wait();

                dbgio_flush();
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_B,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        vdp1_sync_interval_set(-1);

        vdp1_env_default_set();

        vdp2_sprite_priority_set(0, 6);

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();
}

static size_t
_texture_load(texture_t *textures, uint32_t slot, const picture_t *picture, vdp1_vram_t texture_base)
{
        texture_t * const texture = &textures[slot];

        texture->size       = TEXTURE_SIZE(picture->width, picture->height);
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
