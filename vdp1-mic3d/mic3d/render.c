/*
 * Copyright (c) 2006-2022
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 * Mic
 * Shazz
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#include "state.h"

#define SCREEN_RATIO FIX16(SCREEN_WIDTH / (float)SCREEN_HEIGHT)

#define MIN_FOV_ANGLE DEG2ANGLE( 60.0f)
#define MAX_FOV_ANGLE DEG2ANGLE(120.0f)

static void _rotate(point_t *out, const point_t *in, fix16_t angle, uint32_t points_count);
static void _project(const camera_t *camera, point_t *out, const point_t *in, uint32_t points_count);
static void _sort(void);

static void _render_single(const sort_single_t *single);

void
__render_init(void)
{
        __state.render->render_mesh = NULL;
        __state.render->render_mesh_top = __state.render->render_meshes_pool;
        __state.render->cmdts = NULL;
        __state.render->total_points_count = 0;
        __state.render->total_polygons_count = 0;

        render_perspective_set(DEG2ANGLE(90.0f));
}

void
render_start(void)
{
        __render_init();
        __sort_start();
}

void
render_mesh_start(const mesh_t *mesh)
{
        __state.render->render_mesh = __state.render->render_mesh_top;
        __state.render->render_mesh_top++;

        __state.render->render_mesh->mesh = mesh;

        __state.render->render_mesh->mesh = mesh;
        __state.render->render_mesh->in_points = mesh->points;
        __state.render->render_mesh->out_points = &__state.render->points_pool[__state.render->total_points_count];
        __state.render->render_mesh->polygons_count = mesh->polygons_count;
        __state.render->total_points_count += mesh->points_count;
}

void
render_perspective_set(angle_t fov_angle)
{
        fov_angle = fix16_clamp(fov_angle, MIN_FOV_ANGLE, MAX_FOV_ANGLE);

        /* Since angle_t already divides by 360, just multiply by 2π */
        const fix16_t hfov_angle = fix16_mul(fov_angle, FIX16_2PI) >> 1;

        __state.render->view_distance = fix16_mul(FIX16(0.5f * (SCREEN_WIDTH - 1)), fix16_tan(hfov_angle));
}

void
render_mesh_translate(fix16_t x, fix16_t y, fix16_t z)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        const point_t * const in = render_mesh->in_points;
        point_t * const out = render_mesh->out_points;

        for (uint32_t i = 0; i < render_mesh->mesh->points_count; i++) {
                out[i].x = in[i].x + x;
                out[i].y = in[i].y + y;
                out[i].z = in[i].z + z;
        }

        render_mesh->in_points = out;
}

void
render_mesh_rotate(fix16_t angle)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        const point_t * const in = render_mesh->in_points;
        point_t * const out = render_mesh->out_points;

        _rotate(out, in, angle, render_mesh->mesh->points_count);

        render_mesh->in_points = out;
}

void
render_mesh_transform(const camera_t *camera)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        const point_t * const in = render_mesh->in_points;
        point_t * const out = render_mesh->out_points;

        _project(camera, out, in, render_mesh->mesh->points_count);

        _sort();

        __state.render->total_polygons_count += render_mesh->mesh->polygons_count;
}

void
render_process(void)
{
        __state.render->cmdts = __state.render->cmdts_pool;

        __sort_iterate(_render_single);

        vdp1_cmdt_t * const cmdt = __state.render->cmdts;
        __state.render->cmdts++;

        vdp1_cmdt_end_set(cmdt);

        __state.render->render_mesh_top = __state.render->render_meshes_pool;
}

void
render(uint32_t cmdt_index)
{
        const uint32_t count =
            __state.render->cmdts - __state.render->cmdts_pool;

        vdp1_sync_cmdt_put(__state.render->cmdts_pool, count, cmdt_index);
}

static void
_rotate(point_t *out, const point_t *in, fix16_t angle, uint32_t points_count)
{
        const int32_t bradians = fix16_int16_muls(angle, FIX16(FIX16_LUT_SIN_TABLE_COUNT));

        const int32_t sin = fix16_bradians_sin(bradians);
        const int32_t cos = fix16_bradians_cos(bradians);

        for (uint32_t i = 0; i < points_count; i++) {
                /* About X */
                out[i].y = (fix16_mul(in[i].y, cos) - fix16_mul(in[i].z, sin));
                out[i].z = fix16_mul(in[i].y, sin) + fix16_mul(in[i].z, cos);

                /* About Y */
                out[i].x = fix16_mul(in[i].x, cos) - fix16_mul(out[i].z, sin);
                out[i].z = fix16_mul(in[i].x, sin) + fix16_mul(out[i].z, cos);

                /* About Z */
                const int32_t tmp = out[i].x;
                out[i].x = fix16_mul(out[i].x, cos) - fix16_mul(out[i].y, sin);
                out[i].y = fix16_mul(tmp, sin) + fix16_mul(out[i].y, cos);
        }

}

static void
_project(const camera_t *camera, point_t *out, const point_t *in, uint32_t points_count)
{
        for (uint32_t i = 0; i < points_count; i++) {
                const fix16_t in_x = -camera->position.x + in[i].x;
                const fix16_t in_y = -camera->position.y + in[i].y;
                const fix16_t in_z = -camera->position.z + in[i].z;

                cpu_divu_fix16_set(__state.render->view_distance, in_z);

                out[i].z = cpu_divu_quotient_get();

                out[i].x = fix16_mul(out[i].z, in_x);
                out[i].y = fix16_mul(out[i].z, fix16_mul(SCREEN_RATIO, in_y));
        }
}

static void
_sort(void)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        const point_t * const in = render_mesh->in_points;

        for (uint32_t i = 0; i < render_mesh->mesh->polygons_count; i++) {
                const  polygon_t * const polygon = &render_mesh->mesh->polygons[i];

                const fix16_t center_z = in[polygon->p0].z +
                                         in[polygon->p1].z +
                                         in[polygon->p2].z +
                                         in[polygon->p3].z;

                __sort_insert(render_mesh, polygon, center_z);
        }
}

static void
_render_single(const sort_single_t *single)
{
        vdp1_cmdt_t * const cmdt = __state.render->cmdts;

        __state.render->cmdts++;

        const rgb1555_t color = RGB1555(1, 15, 15, 15);

        vdp1_cmdt_draw_mode_t draw_mode = {
                .raw = 0x0000
        };

        /* Set the vertices directly as we have to cast from
         * int32_t to int16_t */

        const  polygon_t * const polygon = single->polygon;
        const render_mesh_t * const render_mesh = single->render_mesh;

        const point_t * const in = render_mesh->in_points;

        cmdt->cmd_xa = fix16_int32_to(in[polygon->p0].x);
        cmdt->cmd_ya = fix16_int32_to(in[polygon->p0].y);

        cmdt->cmd_xb = fix16_int32_to(in[polygon->p3].x);
        cmdt->cmd_yb = fix16_int32_to(in[polygon->p3].y);

        cmdt->cmd_xc = fix16_int32_to(in[polygon->p2].x);
        cmdt->cmd_yc = fix16_int32_to(in[polygon->p2].y);

        cmdt->cmd_xd = fix16_int32_to(in[polygon->p1].x);
        cmdt->cmd_yd = fix16_int32_to(in[polygon->p1].y);

        vdp1_cmdt_polyline_set(cmdt);
        vdp1_cmdt_param_draw_mode_set(cmdt, draw_mode);
        vdp1_cmdt_param_color_set(cmdt, color);
}
