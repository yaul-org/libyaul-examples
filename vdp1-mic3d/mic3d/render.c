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

static void _x_rotate(point_t *out_points, const point_t *in_points, fix16_t angle, uint32_t points_count);
static void _xyz_rotate(point_t *out_points, const point_t *in_points, fix16_t angle, uint32_t points_count);
static void _transform(const camera_t *camera, render_mesh_t *render_mesh);
static void _sort(void);

static bool _backface_cull_test(const int16_vec2_t *p0, const int16_vec2_t *p1, const int16_vec2_t *p2);
static bool _frustrum_cull_test(const clip_flags_t *clip_flags);

static clip_flags_t _clip_flags_calculate(const point_t *point);

static void _render_single(const sort_single_t *single);

static inline void __always_inline
_render_meshes_reset(void)
{
        __state.render->render_mesh_top = __state.render->render_meshes_pool;
}

static inline render_mesh_t * __always_inline
_render_meshes_alloc(void)
{
        render_mesh_t * const render_mesh = __state.render->render_mesh_top;

        __state.render->render_mesh_top++;

        return render_mesh;
}

void
__render_init(void)
{
        __state.render->render_mesh = NULL;
        __state.render->cmdts = NULL;
        __state.render->total_points_count = 0;
        __state.render->total_polygons_count = 0;

        _render_meshes_reset();

        render_perspective_set(DEG2ANGLE(90.0f));

        /* XXX: Actually calculate this */
        __state.render->clip_factor = FIX16(1.0f);
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
        render_mesh_t * const render_mesh = _render_meshes_alloc();

        render_mesh->mesh = mesh;

        render_mesh->in_points = mesh->points;
        render_mesh->out_points = &__state.render->points_pool[__state.render->total_points_count];
        render_mesh->screen_points = &__state.render->screen_points_pool[__state.render->total_points_count];

        render_mesh->in_polygons = mesh->polygons;
        render_mesh->out_polygons = &__state.render->polygons_pool[__state.render->total_polygons_count];

        render_mesh->polygons_count = 0;

        __state.render->render_mesh = render_mesh;

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

        const point_t * const in_points = render_mesh->in_points;
        point_t * const out_points = render_mesh->out_points;

        for (uint32_t i = 0; i < render_mesh->mesh->points_count; i++) {
                out_points[i].x = in_points[i].x + x;
                out_points[i].y = in_points[i].y + y;
                out_points[i].z = in_points[i].z + z;
        }

        render_mesh->in_points = out_points;
}

void
render_mesh_rotate_x(angle_t angle)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        const point_t * const in_points = render_mesh->in_points;
        point_t * const out_points = render_mesh->out_points;

        _x_rotate(out_points, in_points, angle, render_mesh->mesh->points_count);

        render_mesh->in_points = out_points;
}

void
render_mesh_rotate(fix16_t angle)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        const point_t * const in_points = render_mesh->in_points;
        point_t * const out_points = render_mesh->out_points;

        _xyz_rotate(out_points, in_points, angle, render_mesh->mesh->points_count);

        render_mesh->in_points = out_points;
}

void
render_mesh_transform(const camera_t *camera)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        point_t * const out_points = render_mesh->out_points;
        int16_vec2_t * const out_screen_points = render_mesh->screen_points;

        _transform(camera, render_mesh);

        const polygon_t * const in_polygons = render_mesh->in_polygons;
        polygon_meta_t * const out_polygons = render_mesh->out_polygons;

        uint32_t polygon_index;
        polygon_index = 0;

        for (uint32_t i = 0; i < render_mesh->mesh->polygons_count; i++) {
                const int16_vec2_t * const screen_p0 = &out_screen_points[in_polygons[i].p0];
                const int16_vec2_t * const screen_p1 = &out_screen_points[in_polygons[i].p1];
                const int16_vec2_t * const screen_p2 = &out_screen_points[in_polygons[i].p2];

                if ((_backface_cull_test(screen_p0, screen_p1, screen_p2))) {
                        continue;
                }

                const point_t * const view_p0 = &out_points[in_polygons[i].p0];
                const point_t * const view_p1 = &out_points[in_polygons[i].p1];
                const point_t * const view_p2 = &out_points[in_polygons[i].p2];
                const point_t * const view_p3 = &out_points[in_polygons[i].p3];

                clip_flags_t clip_flags[4];

                clip_flags[0] = _clip_flags_calculate(view_p0);
                clip_flags[1] = _clip_flags_calculate(view_p1);
                clip_flags[2] = _clip_flags_calculate(view_p2);
                clip_flags[3] = _clip_flags_calculate(view_p3);

                if ((_frustrum_cull_test(clip_flags))) {
                        continue;
                }

                out_polygons[polygon_index].polygon = &in_polygons[i];
                polygon_index++;
        }

        render_mesh->polygons_count = polygon_index;

        _sort();

        __state.render->total_polygons_count += render_mesh->polygons_count;
}

void
render_process(void)
{
        __state.render->cmdts = __state.render->cmdts_pool;

        __sort_iterate(_render_single);

        vdp1_cmdt_t * const cmdt = __state.render->cmdts;
        __state.render->cmdts++;

        vdp1_cmdt_end_set(cmdt);

        _render_meshes_reset();
}

void
render(uint32_t cmdt_index)
{
        const uint32_t count =
            __state.render->cmdts - __state.render->cmdts_pool;

        vdp1_sync_cmdt_put(__state.render->cmdts_pool, count, cmdt_index);
}

static void
_x_rotate(point_t *out_points, const point_t *in_points, fix16_t angle, uint32_t points_count)
{
        const int32_t bradians = fix16_int16_muls(angle, FIX16(FIX16_LUT_SIN_TABLE_COUNT));

        const int32_t sin = fix16_bradians_sin(bradians);
        const int32_t cos = fix16_bradians_cos(bradians);

        for (uint32_t i = 0; i < points_count; i++) {
                /* About X */
                out_points[i].x = in_points[i].x;
                out_points[i].y = (fix16_mul(in_points[i].y, cos) - fix16_mul(in_points[i].z, sin));
                out_points[i].z = fix16_mul(in_points[i].y, sin) + fix16_mul(in_points[i].z, cos);
        }

}

static void
_xyz_rotate(point_t *out_points, const point_t *in_points, fix16_t angle, uint32_t points_count)
{
        const int32_t bradians = fix16_int16_muls(angle, FIX16(FIX16_LUT_SIN_TABLE_COUNT));

        const int32_t sin = fix16_bradians_sin(bradians);
        const int32_t cos = fix16_bradians_cos(bradians);

        for (uint32_t i = 0; i < points_count; i++) {
                /* About X */
                out_points[i].y = (fix16_mul(in_points[i].y, cos) - fix16_mul(in_points[i].z, sin));
                out_points[i].z = fix16_mul(in_points[i].y, sin) + fix16_mul(in_points[i].z, cos);

                /* About Y */
                out_points[i].x = fix16_mul(in_points[i].x, cos) - fix16_mul(out_points[i].z, sin);
                out_points[i].z = fix16_mul(in_points[i].x, sin) + fix16_mul(out_points[i].z, cos);

                /* About Z */
                const int32_t tmp = out_points[i].x;
                out_points[i].x = fix16_mul(out_points[i].x, cos) - fix16_mul(out_points[i].y, sin);
                out_points[i].y = fix16_mul(tmp, sin) + fix16_mul(out_points[i].y, cos);
        }

}

static void
_transform(const camera_t *camera, render_mesh_t *render_mesh)
{
        const point_t * const in_points = render_mesh->in_points;
        point_t * const out_points = render_mesh->out_points;
        int16_vec2_t * const out_screen_points = render_mesh->screen_points;

        for (uint32_t i = 0; i < render_mesh->mesh->points_count; i++) {
                out_points[i].x = -camera->position.x + in_points[i].x;
                out_points[i].y = -camera->position.y + in_points[i].y;
                out_points[i].z = -camera->position.z + in_points[i].z;

                cpu_divu_fix16_set(__state.render->view_distance, out_points[i].z);

                const fix16_t inv_z = cpu_divu_quotient_get();

                out_screen_points[i].x = fix16_int16_muls(inv_z, out_points[i].x);
                out_screen_points[i].y = fix16_int16_muls(inv_z, fix16_mul(SCREEN_RATIO, out_points[i].y));
        }
}

static void
_sort(void)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        const point_t * const out_points = render_mesh->out_points;

        for (uint32_t i = 0; i < render_mesh->polygons_count; i++) {
                const polygon_meta_t * const meta_polygon = &render_mesh->out_polygons[i];

                const fix16_t center_z = out_points[meta_polygon->polygon->p0].z +
                                         out_points[meta_polygon->polygon->p1].z +
                                         out_points[meta_polygon->polygon->p2].z +
                                         out_points[meta_polygon->polygon->p3].z;

                __sort_insert(render_mesh, meta_polygon, center_z);
        }
}

static clip_flags_t
_clip_flags_calculate(const point_t *point)
{
        clip_flags_t clip_flags;
        clip_flags = CLIP_FLAGS_NONE;

        if (point->z < DEPTH_NEAR) {
                clip_flags |= CLIP_FLAGS_NEAR;
        } else if (point->z > DEPTH_FAR) {
                clip_flags |= CLIP_FLAGS_FAR;
        }

        /* One multiplication and a comparison is faster than a dot product */

        /* This is still confusing to me. If FOV=90, then the slope of the right
         * plane (on XZ-axis) is 1. Taking into account a FOV less than 90, we
         * must take tan(theta/2) into account (half-FOV). So:
         * X=tan(theta/2)*Z */
        const fix16_t factor = fix16_mul(__state.render->clip_factor, point->z);
        const fix16_t neg_factor = -factor;

        if (point->x > factor) {
                clip_flags |= CLIP_FLAGS_RIGHT;
        } else if (point->x < neg_factor) {
                clip_flags |= CLIP_FLAGS_LEFT;
        }

        /* Remember, -Y up so this is why clip flags are reversed */
        if (point->y > factor) {
                clip_flags |= CLIP_FLAGS_BOTTOM;
        } else if (point->y < neg_factor) {
                clip_flags |= CLIP_FLAGS_TOP;
        }

        return clip_flags;
}

static bool
_backface_cull_test(const int16_vec2_t *p0, const int16_vec2_t *p1, const int16_vec2_t *p2)
{
        const int32_vec2_t u1 = {
                .x = p2->x - p0->x,
                .y = p2->y - p0->y
        };

        const int32_vec2_t v1 = {
                .x = p1->x - p0->x,
                .y = p1->y - p0->y
        };

        /* Ideally, we only need to do a cross product on one winding order, but
         * in the case of triangles that combine two vertices, it's unknown
         * which two vertices are joined. The easiest solution is to test one
         * winding order, and if it fails, don't bother testing the other
         * winding order */

        const int32_t z1 = (u1.x * v1.y) - (u1.y * v1.x);

        return (z1 < 0);
}

static bool
_frustrum_cull_test(const clip_flags_t *clip_flags)
{
        return ((clip_flags[0] & clip_flags[1] & clip_flags[2] & clip_flags[3]) != CLIP_FLAGS_NONE);
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

        const polygon_meta_t * const meta_polygon = single->polygon;
        const render_mesh_t * const render_mesh = single->render_mesh;

        const int16_vec2_t * const screen_points = render_mesh->screen_points;

        cmdt->cmd_xa = screen_points[meta_polygon->polygon->p0].x;
        cmdt->cmd_ya = screen_points[meta_polygon->polygon->p0].y;

        cmdt->cmd_xb = screen_points[meta_polygon->polygon->p3].x;
        cmdt->cmd_yb = screen_points[meta_polygon->polygon->p3].y;

        cmdt->cmd_xc = screen_points[meta_polygon->polygon->p2].x;
        cmdt->cmd_yc = screen_points[meta_polygon->polygon->p2].y;

        cmdt->cmd_xd = screen_points[meta_polygon->polygon->p1].x;
        cmdt->cmd_yd = screen_points[meta_polygon->polygon->p1].y;

        vdp1_cmdt_polyline_set(cmdt);
        vdp1_cmdt_param_draw_mode_set(cmdt, draw_mode);
        vdp1_cmdt_param_color_set(cmdt, color);
}
