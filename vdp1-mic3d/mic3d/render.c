/*
 * Copyright (c) 2006-2023
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 * Mic
 * Shazz
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <cpu/registers.h>
#include <cpu/divu.h>

#include "state.h"

#define SCREEN_RATIO FIX16(SCREEN_WIDTH / (float)SCREEN_HEIGHT)

#define MIN_FOV_ANGLE DEG2ANGLE( 20.0f)
#define MAX_FOV_ANGLE DEG2ANGLE(120.0f)

static void _view_transform(render_mesh_t *render_mesh);
static void _projection_transform(render_mesh_t *render_mesh);
static void _sort(void);

static fix16_t _depth_min_calculate(fix16_t p0, fix16_t p1, fix16_t p2, fix16_t p3);
static fix16_t _depth_max_calculate(fix16_t p0, fix16_t p1, fix16_t p2, fix16_t p3);
static fix16_t _depth_center_calculate(fix16_t p0, fix16_t p1, fix16_t p2, fix16_t p3);

static bool _backface_cull_test(const int16_vec2_t *p0, const int16_vec2_t *p1, const int16_vec2_t *p2);
static bool _frustrum_cull_test(const clip_flags_t *clip_flags);

static clip_flags_t _clip_flags_calculate(const fix16_vec3_t *point);

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
        extern fix16_vec3_t __pool_points[];
        extern int16_vec2_t __pool_screen_points[];
        extern fix16_t __pool_depth_values[];
        extern polygon_meta_t __pool_polygons[];

        extern vdp1_cmdt_t __pool_cmdts[];

        static render_mesh_t _pool_render_meshes[16];

        __state.render->points_pool = __pool_points;
        __state.render->screen_points_pool = __pool_screen_points;
        __state.render->depth_values_pool = __pool_depth_values;
        __state.render->polygons_pool = __pool_polygons;
        __state.render->cmdts_pool = __pool_cmdts;
        __state.render->render_meshes_pool = _pool_render_meshes;

        render_start();
}

void
render_start(void)
{
        __state.render->render_mesh = NULL;
        __state.render->cmdts = NULL;
        __state.render->total_points_count = 0;
        __state.render->total_polygons_count = 0;

        _render_meshes_reset();

        render_perspective_set(DEG2ANGLE(90.0f));

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
        render_mesh->depth_values = &__state.render->depth_values_pool[__state.render->total_points_count];

        render_mesh->in_polygons = mesh->polygons;
        render_mesh->out_polygons = &__state.render->polygons_pool[__state.render->total_polygons_count];

        render_mesh->polygons_count = 0;

        __state.render->render_mesh = render_mesh;

        __state.render->total_points_count += mesh->points_count;
}

void
render_perspective_set(angle_t fov_angle)
{
        fov_angle = clamp(fov_angle, MIN_FOV_ANGLE, MAX_FOV_ANGLE);

        const angle_t hfov_angle = fov_angle >> 1;

        __state.render->view_distance = fix16_mul(FIX16(0.5f * (SCREEN_WIDTH - 1)), fix16_tan(hfov_angle));
        __state.render->clip_factor = fix16_div(FIX16((float)SCREEN_WIDTH * 0.5f), __state.render->view_distance);
}

void
render_mesh_transform(void)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        fix16_vec3_t * const out_points = render_mesh->out_points;
        int16_vec2_t * const screen_points = render_mesh->screen_points;

        _view_transform(render_mesh);
        _projection_transform(render_mesh);

        const polygon_t * const in_polygons = render_mesh->in_polygons;
        polygon_meta_t * const out_polygons = render_mesh->out_polygons;

        uint32_t polygon_index;
        polygon_index = 0;

        for (uint32_t i = 0; i < render_mesh->mesh->polygons_count; i++) {
                attribute_t attribute = render_mesh->mesh->attributes[i];

                if (attribute.control.plane_type != PLANE_TYPE_DOUBLE) {
                        const int16_vec2_t * const screen_p0 = &screen_points[in_polygons[i].p0];
                        const int16_vec2_t * const screen_p1 = &screen_points[in_polygons[i].p1];
                        const int16_vec2_t * const screen_p2 = &screen_points[in_polygons[i].p2];

                        if ((_backface_cull_test(screen_p0, screen_p1, screen_p2))) {
                                continue;
                        }
                }

                const fix16_vec3_t * const view_p0 = &out_points[in_polygons[i].p0];
                const fix16_vec3_t * const view_p1 = &out_points[in_polygons[i].p1];
                const fix16_vec3_t * const view_p2 = &out_points[in_polygons[i].p2];
                const fix16_vec3_t * const view_p3 = &out_points[in_polygons[i].p3];

                clip_flags_t clip_flags[4];

                clip_flags[0] = _clip_flags_calculate(view_p0);
                clip_flags[1] = _clip_flags_calculate(view_p1);
                clip_flags[2] = _clip_flags_calculate(view_p2);
                clip_flags[3] = _clip_flags_calculate(view_p3);

                if ((_frustrum_cull_test(clip_flags))) {
                        continue;
                }

                /* Since no clip flags are set, disable pre-clipping. This
                 * should help with performance */
                if ((clip_flags[0] | clip_flags[1] | clip_flags[2] | clip_flags[3]) == CLIP_FLAGS_NONE) {
                        attribute.draw_mode.pre_clipping_disable = true;
                }

                out_polygons[polygon_index].index = i;
                out_polygons[polygon_index].attribute = attribute;

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
_view_transform(render_mesh_t *render_mesh)
{
        const fix16_mat_t * const world_matrix = matrix_top();

        fix16_mat_t inv_view_matrix __aligned(16);
        fix16_mat_t matrix __aligned(16);

        __camera_view_invert(&inv_view_matrix);
        fix16_mat_mul(&inv_view_matrix, world_matrix, &matrix);

        const fix16_vec3_t * const m0 = (const fix16_vec3_t *)&matrix.row[0];
        const fix16_vec3_t * const m1 = (const fix16_vec3_t *)&matrix.row[1];
        const fix16_vec3_t * const m2 = (const fix16_vec3_t *)&matrix.row[2];

        const fix16_vec3_t * const in_points = render_mesh->in_points;
        fix16_vec3_t * const out_points = render_mesh->out_points;

        for (uint32_t i = 0; i < render_mesh->mesh->points_count; i++) {
                out_points[i].x = fix16_vec3_dot(m0, &in_points[i]) + matrix.frow[0][3];
                out_points[i].y = fix16_vec3_dot(m1, &in_points[i]) + matrix.frow[1][3];
                out_points[i].z = fix16_vec3_dot(m2, &in_points[i]) + matrix.frow[2][3];
        }

        render_mesh->in_points = out_points;
}

static void
_projection_transform(render_mesh_t *render_mesh)
{
        fix16_vec3_t * const out_points = render_mesh->out_points;
        int16_vec2_t * const screen_points = render_mesh->screen_points;
        fix16_t * const depth_values = render_mesh->depth_values;

        for (uint32_t i = 0; i < render_mesh->mesh->points_count; i++) {
                depth_values[i] = fix16_div(__state.render->view_distance, out_points[i].z);
                screen_points[i].x = fix16_int32_mul(depth_values[i], out_points[i].x);
                screen_points[i].y = fix16_int32_mul(depth_values[i], fix16_mul(SCREEN_RATIO, out_points[i].y));
        }
}

static void
_sort(void)
{
        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        const fix16_vec3_t * const out_points = render_mesh->out_points;

        for (uint32_t i = 0; i < render_mesh->polygons_count; i++) {
                const polygon_meta_t * const meta_polygon = &render_mesh->out_polygons[i];

                const polygon_t * const polygon =
                    &render_mesh->in_polygons[meta_polygon->index];

                const fix16_t p0 = out_points[polygon->p0].z;
                const fix16_t p1 = out_points[polygon->p1].z;
                const fix16_t p2 = out_points[polygon->p2].z;
                const fix16_t p3 = out_points[polygon->p3].z;

                fix16_t z;
                z = FIX16_ZERO;

                switch (meta_polygon->attribute.control.sort_type) {
                default:
                case SORT_TYPE_CENTER:
                        z = _depth_center_calculate(p0, p1, p2, p3);
                        break;
                case SORT_TYPE_MIN:
                        z = _depth_min_calculate(p0, p1, p2, p3);
                        break;
                case SORT_TYPE_MAX:
                        z = _depth_max_calculate(p0, p1, p2, p3);
                        break;
                case SORT_TYPE_BFR:
                        continue;
                }

                /* Dividing by 64 was pulled by the PSXSPX documents */
                const int32_t shifted_z = fix16_int32_mul(z, FIX16(SORT_DEPTH / (float)DEPTH_FAR));

                __sort_insert(render_mesh, meta_polygon, shifted_z);
        }
}

static int32_t
_depth_min_calculate(fix16_t p0, fix16_t p1, fix16_t p2, fix16_t p3)
{
        fix16_t z;
        z = p0;

        z = (p1 < z) ? p1 : z;
        z = (p2 < z) ? p2 : z;
        z = (p3 < z) ? p3 : z;

        return z;
}

static fix16_t
_depth_max_calculate(fix16_t p0, fix16_t p1, fix16_t p2, fix16_t p3)
{
        fix16_t z;
        z = p0;

        z = (p1 > z) ? p1 : z;
        z = (p2 > z) ? p2 : z;
        z = (p3 > z) ? p3 : z;

        return fix16_int32_to(z);
}

static fix16_t
_depth_center_calculate(fix16_t p0, fix16_t p1, fix16_t p2, fix16_t p3)
{
        return ((p0 + p1 + p2 + p3) >> 2);
}

static clip_flags_t
_clip_flags_calculate(const fix16_vec3_t *point)
{
        clip_flags_t clip_flags;
        clip_flags = CLIP_FLAGS_NONE;

        if (point->z < FIX16(DEPTH_NEAR)) {
                clip_flags |= CLIP_FLAGS_NEAR;
        } else if (point->z > FIX16(DEPTH_FAR)) {
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
        const int32_vec2_t a = {
                .x = p2->x - p0->x,
                .y = p2->y - p0->y
        };

        const int32_vec2_t b = {
                .x = p1->x - p0->x,
                .y = p1->y - p0->y
        };

        const int32_t z = (a.x * b.y) - (a.y * b.x);

        return (z < 0);
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

        const polygon_meta_t * const meta_polygon = single->polygon;

        const render_mesh_t * const render_mesh = single->render_mesh;

        const attribute_t attribute = meta_polygon->attribute;

        cmdt->cmd_ctrl &= 0x7FF0;
        cmdt->cmd_ctrl |= attribute.control.raw & 0x3F;

        vdp1_cmdt_draw_mode_set(cmdt, attribute.draw_mode);

        if (attribute.control.use_texture) {
                const texture_t * const textures = tlist_get();
                const texture_t * const texture = &textures[attribute.texture_slot];

                cmdt->cmd_srca = texture->vram_index;
                cmdt->cmd_size = texture->size;
        }

        cmdt->cmd_colr = attribute.palette.raw;

        const polygon_t * const polygon =
            &render_mesh->in_polygons[meta_polygon->index];

        const int16_vec2_t * const screen_points = render_mesh->screen_points;

        cmdt->cmd_xa = screen_points[polygon->p0].x;
        cmdt->cmd_ya = screen_points[polygon->p0].y;

        cmdt->cmd_xb = screen_points[polygon->p3].x;
        cmdt->cmd_yb = screen_points[polygon->p3].y;

        cmdt->cmd_xc = screen_points[polygon->p2].x;
        cmdt->cmd_yc = screen_points[polygon->p2].y;

        cmdt->cmd_xd = screen_points[polygon->p1].x;
        cmdt->cmd_yd = screen_points[polygon->p1].y;

        cmdt->cmd_grda = attribute.shading_slot;
}
