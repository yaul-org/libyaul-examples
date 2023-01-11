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

#include <dbgio.h>

#include <cpu/registers.h>
#include <cpu/divu.h>

#include "internal.h"

#define SCREEN_RATIO       FIX16(SCREEN_WIDTH / (float)SCREEN_HEIGHT)

#define SCREEN_CLIP_LEFT   (-SCREEN_WIDTH / 2)
#define SCREEN_CLIP_RIGHT  (-SCREEN_CLIP_LEFT)
#define SCREEN_CLIP_TOP    (-SCREEN_HEIGHT / 2)
#define SCREEN_CLIP_BOTTOM (-SCREEN_CLIP_TOP)

#define MIN_FOV_ANGLE DEG2ANGLE( 20.0f)
#define MAX_FOV_ANGLE DEG2ANGLE(120.0f)

static void _transform(render_mesh_t *render_mesh);

static int32_t _depth_calculate(const polygon_meta_t *meta_polygon, const fix16_t *view_points);
static fix16_t _depth_min_calculate(const fix16_t *view_points);
static fix16_t _depth_max_calculate(const fix16_t *view_points);
static fix16_t _depth_center_calculate(const fix16_t *view_points);

static bool _backface_cull_test(const int16_vec2_t *screen_points[]);

static void _clip_flags_calculate(const fix16_t *z_value, const int16_vec2_t *screen_points[], clip_flags_t *or_flags, clip_flags_t *and_flags);
static void _clip_flags_nf_calculate(fix16_t z, clip_flags_t *or_flags, clip_flags_t *and_flags);
static void _clip_flags_lrtb_calculate(const int16_vec2_t *screen_point, clip_flags_t *or_flags, clip_flags_t *and_flags);

static void _render_single(const sort_single_t *single);

static perf_counter_t _transform_pc;

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
        extern fix16_t __pool_z_values[];
        extern int16_vec2_t __pool_screen_points[];
        extern fix16_t __pool_depth_values[];
        extern polygon_meta_t __pool_meta_polygons[];

        extern vdp1_cmdt_t __pool_cmdts[];

        static render_mesh_t _pool_render_meshes[16];

        __state.render->z_values_pool = __pool_z_values;
        __state.render->screen_points_pool = __pool_screen_points;
        __state.render->depth_values_pool = __pool_depth_values;
        __state.render->meta_polygons_pool = __pool_meta_polygons;
        __state.render->cmdts_pool = __pool_cmdts;
        __state.render->render_meshes_pool = _pool_render_meshes;

        __state.render->render_flags = RENDER_FLAGS_NONE;

        render_start();

        __perf_counter_init(&_transform_pc);
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
render_enable(render_flags_t flags)
{
        __state.render->render_flags |= flags;
}

void
render_disable(render_flags_t flags)
{
        __state.render->render_flags &= ~flags;
}

void
render_mesh_start(const mesh_t *mesh)
{
        render_mesh_t * const render_mesh = _render_meshes_alloc();

        render_mesh->mesh = mesh;

        render_mesh->z_values = &__state.render->z_values_pool[__state.render->total_points_count];
        render_mesh->screen_points = &__state.render->screen_points_pool[__state.render->total_points_count];
        render_mesh->depth_values = &__state.render->depth_values_pool[__state.render->total_points_count];

        render_mesh->meta_polygons = &__state.render->meta_polygons_pool[__state.render->total_polygons_count];

        render_mesh->polygons_count = 0;

        __state.render->render_mesh = render_mesh;

        __state.render->total_points_count += mesh->points_count;
}

void
render_perspective_set(angle_t fov_angle)
{
        fov_angle = clamp(fov_angle, MIN_FOV_ANGLE, MAX_FOV_ANGLE);

        const angle_t hfov_angle = fov_angle >> 1;
        const fix16_t screen_scale = FIX16(0.5f * (SCREEN_WIDTH - 1));
        const fix16_t tan = fix16_tan(hfov_angle);

        __state.render->view_distance = fix16_mul(screen_scale, tan);
}

void
render_mesh_transform(void)
{
        __perf_counter_start(&_transform_pc);

        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        fix16_t * const z_values = render_mesh->z_values;
        int16_vec2_t * const screen_points = render_mesh->screen_points;

        _transform(render_mesh);

        const polygon_t * const polygons = render_mesh->mesh->polygons;
        polygon_meta_t * const meta_polygons = render_mesh->meta_polygons;

        polygon_meta_t *meta_polygon;
        meta_polygon = meta_polygons;

        for (uint32_t i = 0; i < render_mesh->mesh->polygons_count; i++) {
                attribute_t attribute = render_mesh->mesh->attributes[i];

                const polygon_t * const polygon = &polygons[i];

                const int16_vec2_t * polygon_screen_points[] = {
                        &screen_points[polygon->p0],
                        &screen_points[polygon->p1],
                        &screen_points[polygon->p2],
                        &screen_points[polygon->p3]
                };

                if (attribute.control.plane_type != PLANE_TYPE_DOUBLE) {
                        if ((_backface_cull_test(polygon_screen_points))) {
                                continue;
                        }
                }

                const fix16_t polygon_z_values[] = {
                        z_values[polygon->p0],
                        z_values[polygon->p1],
                        z_values[polygon->p2],
                        z_values[polygon->p3]
                };

                clip_flags_t or_flags;
                clip_flags_t and_flags;

                _clip_flags_calculate(polygon_z_values, polygon_screen_points, &or_flags, &and_flags);

                if (and_flags != CLIP_FLAGS_NONE) {
                        continue;
                }

                /* Since no clip flags are set, disable pre-clipping. This
                 * should help with performance */
                attribute.draw_mode.pre_clipping_disable = (or_flags == CLIP_FLAGS_NONE);

                meta_polygon->index = i;
                meta_polygon->attribute = attribute;

                const int32_t shifted_z = _depth_calculate(meta_polygon, polygon_z_values);

                __sort_insert(render_mesh, meta_polygon, shifted_z);

                meta_polygon++;
        }

        render_mesh->polygons_count = meta_polygon - meta_polygons;

        __light_mesh_transform();

        __state.render->total_polygons_count += render_mesh->polygons_count;

        __perf_counter_end(&_transform_pc);

        char buffer[32];
        __perf_str(_transform_pc.ticks, buffer);

        dbgio_printf("ticks: %5lu, %5lu, %sms\n", _transform_pc.ticks, _transform_pc.max_ticks, buffer);
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

        __light_gst_put();
}

static void
_transform(render_mesh_t *render_mesh)
{
        const fix16_mat43_t * const world_matrix = matrix_top();

        fix16_mat43_t inv_view_matrix __aligned(16);

        __camera_view_invert(&inv_view_matrix);

        fix16_mat43_t matrix __aligned(16);

        fix16_mat43_mul(&inv_view_matrix, world_matrix, &matrix);

        const fix16_vec3_t * const m0 = (const fix16_vec3_t *)&matrix.row[0];
        const fix16_vec3_t * const m1 = (const fix16_vec3_t *)&matrix.row[1];
        const fix16_vec3_t * const m2 = (const fix16_vec3_t *)&matrix.row[2];

        const fix16_vec3_t * const points = render_mesh->mesh->points;
        int16_vec2_t * const screen_points = render_mesh->screen_points;
        fix16_t * const z_values = render_mesh->z_values;
        fix16_t * const depth_values = render_mesh->depth_values;

        for (uint32_t i = 0; i < render_mesh->mesh->points_count; i++) {
                z_values[i] = fix16_vec3_dot(m2, &points[i]) + matrix.frow[2][3];
                depth_values[i] = fix16_div(__state.render->view_distance, z_values[i]);

                screen_points[i].x = fix16_int32_mul(depth_values[i], fix16_vec3_dot(m0, &points[i]) + matrix.frow[0][3]);
                screen_points[i].y = fix16_int32_mul(depth_values[i], fix16_vec3_dot(m1, &points[i]) + matrix.frow[1][3]);
        }
}

static int32_t
_depth_calculate(const polygon_meta_t *meta_polygon, const fix16_t *view_points)
{
        fix16_t z;
        z = FIX16_ZERO;

        switch (meta_polygon->attribute.control.sort_type) {
        default:
        case SORT_TYPE_CENTER:
                z = _depth_center_calculate(view_points);
                break;
        case SORT_TYPE_MIN:
                z = _depth_min_calculate(view_points);
                break;
        case SORT_TYPE_MAX:
                z = _depth_max_calculate(view_points);
                break;
        }

        return fix16_int32_mul(z, FIX16(SORT_DEPTH / (float)DEPTH_FAR));
}

static fix16_t
_depth_min_calculate(const fix16_t *view_points)
{
        fix16_t z;
        z = view_points[0];

        z = (view_points[1] < z) ? view_points[1] : z;
        z = (view_points[2] < z) ? view_points[2] : z;
        z = (view_points[3] < z) ? view_points[3] : z;

        return z;
}

static fix16_t
_depth_max_calculate(const fix16_t *view_points)
{
        fix16_t z;
        z = view_points[0];

        z = (view_points[1] > z) ? view_points[1] : z;
        z = (view_points[2] > z) ? view_points[2] : z;
        z = (view_points[3] > z) ? view_points[3] : z;

        return z;
}

static fix16_t
_depth_center_calculate(const fix16_t *view_points)
{
        return ((view_points[0] + view_points[1] + view_points[2] + view_points[3]) >> 2);
}

static void
_clip_flags_nf_calculate(fix16_t z, clip_flags_t *or_flags, clip_flags_t *and_flags)
{
        clip_flags_t clip_flag;

        clip_flag = (z < FIX16(DEPTH_NEAR)) << CLIP_BIT_NEAR;
        *or_flags |= clip_flag;
        *and_flags &= clip_flag;

        clip_flag = (z > FIX16(DEPTH_FAR)) << CLIP_BIT_FAR;
        *or_flags |= clip_flag;
        *and_flags &= clip_flag;
}

static void
_clip_flags_lrtb_calculate(const int16_vec2_t *screen_point, clip_flags_t *or_flags, clip_flags_t *and_flags)
{
        clip_flags_t clip_flag;

        clip_flag = (screen_point->x < SCREEN_CLIP_LEFT) << CLIP_BIT_LEFT;
        *or_flags |= clip_flag;
        *and_flags &= clip_flag;

        clip_flag = (screen_point->x > SCREEN_CLIP_RIGHT) << CLIP_BIT_RIGHT;
        *or_flags |= clip_flag;
        *and_flags &= clip_flag;

        /* Remember, -Y up so this is why the top and bottom clip flags are
         * reversed */
        clip_flag = (screen_point->y < SCREEN_CLIP_TOP) << CLIP_BIT_TOP;
        *or_flags |= clip_flag;
        *and_flags &= clip_flag;

        clip_flag = (screen_point->y > SCREEN_CLIP_BOTTOM) << CLIP_BIT_BOTTOM;
        *or_flags |= clip_flag;
        *and_flags &= clip_flag;
}

static bool
_backface_cull_test(const int16_vec2_t *screen_points[])
{
        const int32_vec2_t a = {
                .x = screen_points[2]->x - screen_points[0]->x,
                .y = screen_points[2]->y - screen_points[0]->y
        };

        const int32_vec2_t b = {
                .x = screen_points[1]->x - screen_points[0]->x,
                .y = screen_points[1]->y - screen_points[0]->y
        };

        const int32_t z = (a.x * b.y) - (a.y * b.x);

        return (z < 0);
}

static void
_clip_flags_calculate(const fix16_t *z_values, const int16_vec2_t *screen_points[], clip_flags_t *or_flags, clip_flags_t *and_flags)
{
        *or_flags = CLIP_FLAGS_NONE;
        *and_flags = CLIP_FLAGS_NONE;

        _clip_flags_nf_calculate(z_values[0], or_flags, and_flags);
        _clip_flags_nf_calculate(z_values[1], or_flags, and_flags);
        _clip_flags_nf_calculate(z_values[2], or_flags, and_flags);
        _clip_flags_nf_calculate(z_values[3], or_flags, and_flags);

        if (and_flags == CLIP_FLAGS_NONE) {
                _clip_flags_lrtb_calculate(screen_points[0], or_flags, and_flags);
                _clip_flags_lrtb_calculate(screen_points[1], or_flags, and_flags);
                _clip_flags_lrtb_calculate(screen_points[2], or_flags, and_flags);
                _clip_flags_lrtb_calculate(screen_points[3], or_flags, and_flags);
        }
}

static void
_render_single(const sort_single_t *single)
{
        vdp1_cmdt_t * const cmdt = __state.render->cmdts;

        __state.render->cmdts++;

        const polygon_meta_t * const meta_polygon = single->polygon;

        const render_mesh_t * const render_mesh = single->render_mesh;

        const attribute_t attribute = meta_polygon->attribute;

        cmdt->cmd_ctrl = attribute.control.raw & 0x3F;
        cmdt->cmd_pmod = attribute.draw_mode.raw;

        if (attribute.control.use_texture) {
                const texture_t * const textures = tlist_get();
                const texture_t * const texture = &textures[attribute.texture_slot];

                cmdt->cmd_srca = texture->vram_index;
                cmdt->cmd_size = texture->size;
        }

        cmdt->cmd_colr = attribute.palette.raw;

        const polygon_t * const polygon =
            &render_mesh->mesh->polygons[meta_polygon->index];

        const int16_vec2_t * const screen_points = render_mesh->screen_points;

        cmdt->cmd_xa = screen_points[polygon->p0].x;
        cmdt->cmd_ya = screen_points[polygon->p0].y;

        cmdt->cmd_xb = screen_points[polygon->p1].x;
        cmdt->cmd_yb = screen_points[polygon->p1].y;

        cmdt->cmd_xc = screen_points[polygon->p2].x;
        cmdt->cmd_yc = screen_points[polygon->p2].y;

        cmdt->cmd_xd = screen_points[polygon->p3].x;
        cmdt->cmd_yd = screen_points[polygon->p3].y;

        cmdt->cmd_grda = attribute.shading_slot;
}
