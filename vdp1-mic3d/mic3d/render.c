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
#include <cpu/intc.h>

#include "internal.h"

#define SCREEN_RATIO       FIX16(SCREEN_WIDTH / (float)SCREEN_HEIGHT)

#define SCREEN_CLIP_LEFT   (-SCREEN_WIDTH / 2)
#define SCREEN_CLIP_RIGHT  ( SCREEN_WIDTH / 2)
#define SCREEN_CLIP_TOP    (-SCREEN_HEIGHT / 2)
#define SCREEN_CLIP_BOTTOM ( SCREEN_HEIGHT / 2)

#define MIN_FOV_ANGLE DEG2ANGLE( 20.0f)
#define MAX_FOV_ANGLE DEG2ANGLE(120.0f)

#define NEAR_LEVEL_MIN 1U
#define NEAR_LEVEL_MAX 8U

typedef struct {
        attribute_t attribute;
        int16_vec2_t screen_points[4];
        fix16_t z_values[4];
        clip_flags_t clip_flags[4];
        clip_flags_t and_flags;
        clip_flags_t or_flags;
} __aligned(4) transform_t;

static void _transform(void);

static fix16_t _depth_calculate(const transform_t *transform);
static fix16_t _depth_min_calculate(const fix16_t *z_values);
static fix16_t _depth_max_calculate(const fix16_t *z_values);
static fix16_t _depth_center_calculate(const fix16_t *z_values);

static bool _backface_cull_test(const transform_t *transform);

static void _clip_flags_calculate(transform_t *transform);
static void _clip_flags_lrtb_calculate(const int16_vec2_t screen_point, clip_flags_t *clip_flag);

static void _render_single(const sort_single_t *single);

static vdp1_cmdt_t *_cmdts_alloc(void);
static void _cmdts_reset(void);
static vdp1_link_t _cmdt_link_calculate(const vdp1_cmdt_t *cmdt);
static void _cmdt_process(vdp1_cmdt_t *cmdt, const transform_t *transform);

static perf_counter_t _transform_pc;

void
__render_init(void)
{
        extern fix16_t __pool_z_values[];
        extern int16_vec2_t __pool_screen_points[];
        extern fix16_t __pool_depth_values[];

        extern vdp1_cmdt_t __pool_cmdts[];

        __state.render->z_values_pool = __pool_z_values;
        __state.render->screen_points_pool = __pool_screen_points;
        __state.render->depth_values_pool = __pool_depth_values;
        __state.render->cmdts_pool = __pool_cmdts;

        __state.render->render_flags = RENDER_FLAGS_NONE;

        render_perspective_set(DEG2ANGLE(90.0f));
        render_near_level_set(7);
        render_far_set(FIX16(1024));

        render_start();

        __perf_counter_init(&_transform_pc);
}

void
render_start(void)
{
        __state.render->cmdt_count = 0;

        _cmdts_reset();

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
render_perspective_set(angle_t fov_angle)
{
        fov_angle = clamp(fov_angle, MIN_FOV_ANGLE, MAX_FOV_ANGLE);

        const angle_t hfov_angle = fov_angle >> 1;
        const fix16_t screen_scale = FIX16(0.5f * (SCREEN_WIDTH - 1));
        const fix16_t tan = fix16_tan(hfov_angle);

        __state.render->view_distance = fix16_mul(screen_scale, tan);
}

void
render_near_level_set(uint32_t level)
{
        __state.render->near = __state.render->view_distance;

        const uint32_t clamped_level =
            clamp(level + 1, NEAR_LEVEL_MIN, NEAR_LEVEL_MAX);

        for (int32_t i = clamped_level; i > 0; i--) {
                __state.render->near >>= 1;
        }
}

void
render_far_set(fix16_t far)
{
        __state.render->far = fix16_clamp(far, __state.render->near, FIX16(2048.0f));
        __state.render->sort_scale = fix16_div(FIX16(SORT_DEPTH - 1), __state.render->far);
}

void
render_mesh_transform(const mesh_t *mesh)
{
        const uint32_t sr_mask = cpu_intc_mask_get();
        cpu_intc_mask_set(15);

        __state.render->mesh = mesh;

        __perf_counter_start(&_transform_pc);

        fix16_t * const z_values = __state.render->z_values_pool;
        int16_vec2_t * const screen_points = __state.render->screen_points_pool;

        _transform();
        __light_transform();

        const polygon_t * const polygons = __state.render->mesh->polygons;

        transform_t transform __aligned(16);

        for (uint32_t i = 0; i < __state.render->mesh->polygons_count; i++) {
                transform.attribute = __state.render->mesh->attributes[i];

                const polygon_t * const polygon = &polygons[i];

                transform.screen_points[0] = screen_points[polygon->p0];
                transform.screen_points[1] = screen_points[polygon->p1];
                transform.screen_points[2] = screen_points[polygon->p2];
                transform.screen_points[3] = screen_points[polygon->p3];

                if (transform.attribute.control.plane_type != PLANE_TYPE_DOUBLE) {
                        if ((_backface_cull_test(&transform))) {
                                continue;
                        }
                }

                transform.z_values[0] = z_values[polygon->p0];
                transform.z_values[1] = z_values[polygon->p1];
                transform.z_values[2] = z_values[polygon->p2];
                transform.z_values[3] = z_values[polygon->p3];

                const fix16_t depth_z = _depth_calculate(&transform);

                /* Cull polygons intersecting with the near plane */
                if ((depth_z < __state.render->near)) {
                        continue;
                }

                _clip_flags_calculate(&transform);

                /* Cull if the polygon is entirely off screen */
                if (transform.and_flags != CLIP_FLAGS_NONE) {
                        continue;
                }

                if (transform.or_flags == CLIP_FLAGS_NONE) {
                        /* If no clip flags are set, disable pre-clipping. This
                         * should help with performance */
                        transform.attribute.draw_mode.pre_clipping_disable = true;
                } else {
                        /* XXX: Orient vertices here */
                }

                __light_polygon_process(polygon, &transform.attribute);

                vdp1_cmdt_t * const cmdt = _cmdts_alloc();
                const vdp1_link_t cmdt_link = _cmdt_link_calculate(cmdt);

                _cmdt_process(cmdt, &transform);

                const int32_t scaled_z = fix16_int32_mul(depth_z, __state.render->sort_scale);

                __sort_insert(cmdt_link, scaled_z);

                __state.render->cmdt_count++;
        }

        __perf_counter_end(&_transform_pc);

        char buffer[32];
        __perf_str(_transform_pc.ticks, buffer);

        dbgio_printf("%lu\n", __state.render->cmdt_count);
        dbgio_printf("ticks: %5lu, %5lu, %sms\n", _transform_pc.ticks, _transform_pc.max_ticks, buffer);

        cpu_intc_mask_set(sr_mask);
}

void
render(uint32_t subr_index, uint32_t cmdt_index)
{
        vdp1_cmdt_t * const subr_cmdt = (vdp1_cmdt_t *)VDP1_CMD_TABLE(subr_index, 0);

        if (__state.render->cmdt_count == 0) {
                vdp1_cmdt_link_type_set(subr_cmdt, VDP1_CMDT_LINK_TYPE_JUMP_NEXT);

                return;
        }

        __state.render->sort_cmdt = subr_cmdt;
        __state.render->sort_link = cmdt_index;

        /* Set as a subroutine call */
        vdp1_cmdt_link_type_set(subr_cmdt, VDP1_CMDT_LINK_TYPE_JUMP_CALL);

        __sort_iterate(_render_single);

        vdp1_cmdt_t * const end_cmdt = __state.render->sort_cmdt;

        /* Set to return from subroutine */
        vdp1_cmdt_link_type_set(end_cmdt, VDP1_CMDT_LINK_TYPE_JUMP_RETURN);

        vdp1_sync_cmdt_put(__state.render->cmdts_pool, __state.render->cmdt_count, __state.render->sort_link);

        __light_gst_put();
}

static void
_transform(void)
{
        const fix16_mat43_t * const world_matrix = matrix_top();

        fix16_mat43_t inv_view_matrix __aligned(16);

        __camera_view_invert(&inv_view_matrix);

        fix16_mat43_t matrix __aligned(16);

        fix16_mat43_mul(&inv_view_matrix, world_matrix, &matrix);

        const fix16_vec3_t * const m0 = (const fix16_vec3_t *)&matrix.row[0];
        const fix16_vec3_t * const m1 = (const fix16_vec3_t *)&matrix.row[1];
        const fix16_vec3_t * const m2 = (const fix16_vec3_t *)&matrix.row[2];

        const fix16_vec3_t * const points = __state.render->mesh->points;
        int16_vec2_t * const screen_points = __state.render->screen_points_pool;
        fix16_t * const z_values = __state.render->z_values_pool;
        fix16_t * const depth_values = __state.render->depth_values_pool;

        for (uint32_t i = 0; i < __state.render->mesh->points_count; i++) {
                z_values[i] = fix16_vec3_dot(m2, &points[i]) + matrix.frow[2][3];
                depth_values[i] = fix16_div(__state.render->view_distance, fix16_max(z_values[i], __state.render->near));

                screen_points[i].x = fix16_int32_mul(depth_values[i], fix16_vec3_dot(m0, &points[i]) + matrix.frow[0][3]);
                screen_points[i].y = fix16_int32_mul(depth_values[i], fix16_vec3_dot(m1, &points[i]) + matrix.frow[1][3]);
        }
}

static fix16_t
_depth_calculate(const transform_t *transform)
{
        switch (transform->attribute.control.sort_type) {
        default:
        case SORT_TYPE_CENTER:
                return _depth_center_calculate(transform->z_values);
        case SORT_TYPE_MIN:
                return _depth_min_calculate(transform->z_values);
        case SORT_TYPE_MAX:
                return _depth_max_calculate(transform->z_values);
        }
}

static fix16_t
_depth_min_calculate(const fix16_t *z_values)
{
        return fix16_min(fix16_min(z_values[0], z_values[1]),
                         fix16_min(z_values[2], z_values[3]));
}

static fix16_t
_depth_max_calculate(const fix16_t *z_values)
{
        return fix16_max(fix16_max(z_values[0], z_values[1]),
                         fix16_max(z_values[2], z_values[3]));
}

static fix16_t
_depth_center_calculate(const fix16_t *z_values)
{
        return ((z_values[0] + z_values[1] + z_values[2] + z_values[3]) >> 2);
}

static void
_clip_flags_lrtb_calculate(const int16_vec2_t screen_point, clip_flags_t *clip_flag)
{
        *clip_flag  = (screen_point.x <   SCREEN_CLIP_LEFT) << CLIP_BIT_LEFT;
        *clip_flag |= (screen_point.x >  SCREEN_CLIP_RIGHT) << CLIP_BIT_RIGHT;
        /* Remember, -Y up so this is why the top and bottom clip flags are
         * reversed */
        *clip_flag |= (screen_point.y <    SCREEN_CLIP_TOP) << CLIP_BIT_TOP;
        *clip_flag |= (screen_point.y > SCREEN_CLIP_BOTTOM) << CLIP_BIT_BOTTOM;
}

static bool
_backface_cull_test(const transform_t *transform)
{
        const int32_vec2_t a = {
                .x = transform->screen_points[2].x - transform->screen_points[0].x,
                .y = transform->screen_points[2].y - transform->screen_points[0].y
        };

        const int32_vec2_t b = {
                .x = transform->screen_points[1].x - transform->screen_points[0].x,
                .y = transform->screen_points[1].y - transform->screen_points[0].y
        };

        const int32_t z = (a.x * b.y) - (a.y * b.x);

        return (z < 0);
}

static void
_clip_flags_calculate(transform_t *transform)
{
        _clip_flags_lrtb_calculate(transform->screen_points[0], &transform->clip_flags[0]);
        _clip_flags_lrtb_calculate(transform->screen_points[1], &transform->clip_flags[1]);
        _clip_flags_lrtb_calculate(transform->screen_points[2], &transform->clip_flags[2]);
        _clip_flags_lrtb_calculate(transform->screen_points[3], &transform->clip_flags[3]);

        transform->and_flags = transform->clip_flags[0] &
                               transform->clip_flags[1] &
                               transform->clip_flags[2] &
                               transform->clip_flags[3];

        transform->or_flags = transform->clip_flags[0] |
                              transform->clip_flags[1] |
                              transform->clip_flags[2] |
                              transform->clip_flags[3];
}

static void
_render_single(const sort_single_t *single)
{
        vdp1_cmdt_link_set(__state.render->sort_cmdt, single->link + __state.render->sort_link);

        /* Point to the next command table */
        __state.render->sort_cmdt = &__state.render->cmdts_pool[single->link];
}

static vdp1_cmdt_t *
_cmdts_alloc(void)
{
        vdp1_cmdt_t * const cmdt = __state.render->cmdts;

        __state.render->cmdts++;

        return cmdt;
}

static void
_cmdts_reset(void)
{
        __state.render->cmdts = __state.render->cmdts_pool;
}

static vdp1_link_t
_cmdt_link_calculate(const vdp1_cmdt_t *cmdt)
{
        return (cmdt - __state.render->cmdts_pool);
}

static void
_cmdt_process(vdp1_cmdt_t *cmdt, const transform_t *transform)
{
        cmdt->cmd_ctrl = VDP1_CMDT_LINK_TYPE_JUMP_ASSIGN | (transform->attribute.control.raw & 0x3F);
        cmdt->cmd_pmod = transform->attribute.draw_mode.raw;

        if (transform->attribute.control.use_texture) {
                const texture_t * const textures = tlist_get();
                const texture_t * const texture = &textures[transform->attribute.texture_slot];

                cmdt->cmd_srca = texture->vram_index;
                cmdt->cmd_size = texture->size;
        }

        cmdt->cmd_colr = transform->attribute.palette.raw;

        cmdt->cmd_vertices[0] = transform->screen_points[0];
        cmdt->cmd_vertices[1] = transform->screen_points[1];
        cmdt->cmd_vertices[2] = transform->screen_points[2];
        cmdt->cmd_vertices[3] = transform->screen_points[3];

        cmdt->cmd_grda = transform->attribute.shading_slot;
}
