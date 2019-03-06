/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <string.h>

#include "cube.h"

#define MODEL_TRANSFORMATIONS   1 /* 0: No transforms   1: Apply transforms */
#define MODEL_PROJECT           1 /* 0: Upload          1: Upload and project */
#define POLYGON_SORT            1 /* 0: No sort         1: Sort */
#define RENDER                  1 /* 0: No render       1: Render */
#define CULLING                 1 /* 0: No culling      1: Culling */

static uint16_t colors[TEAPOT_POLYGON_CNT] __unused;

static void model_polygon_project(const fix16_vector4_t *, const uint32_t *,
    const fix16_vector4_t *, const uint32_t);

static uint32_t tick = 0;

static void hardware_init(void);
static void vblank_in_handler(irq_mux_handle_t *irq_mux __unused);
static void vblank_out_handler(irq_mux_handle_t *irq_mux __unused);

void
main(void)
{
        hardware_init();

        uint32_t color_idx;
        for (color_idx = 0; color_idx < TEAPOT_POLYGON_CNT; color_idx++) {
                uint32_t idx;
                idx = color_idx;

                uint32_t polygon_idx;
                fix16_vector4_t *polygon[4];
                for (polygon_idx = 0; polygon_idx < 4; polygon_idx++) {
                        uint32_t vertex_idx;
                        vertex_idx = teapot_indices[(4 * idx) + polygon_idx];
                        fix16_vector4_t *vertex;
                        vertex = &teapot_vertices[vertex_idx];

                        polygon[polygon_idx] = vertex;
                }

                fix16_t avg;
                avg = fix16_add(fix16_mul(fix16_add(
                                fix16_add(polygon[0]->z, polygon[1]->z),
                                fix16_add(polygon[2]->z, polygon[3]->z)),
                        F16(1.0f / 4.0f)), F16(0.1f));

                uint16_t color;
                color = fix16_to_int(fix16_add(
                            fix16_mul(fix16_abs(avg), F16(255.0f)),
                            F16(16.0f)));

                colors[color_idx] = RGB888_TO_RGB555(color, color, color);
        }

        ot_init();
        matrix_stack_init();

        matrix_stack_mode(MATRIX_STACK_MODE_PROJECTION);

        fix16_t ratio;
        ratio = F16((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT);

        matrix_stack_orthographic_project(-fix16_mul(F16(1.0f), ratio),
            fix16_mul(F16(1.0f), ratio),
            F16(1.0f), F16(-1.0f),
            F16(1.0f), F16(1.0f));

        matrix_stack_mode(MATRIX_STACK_MODE_MODEL_VIEW);
        matrix_stack_translate(F16(0.0f), F16(0.0f), F16(-10.0f));

        vdp1_cmdt_list_begin(0); {
                struct vdp1_cmdt_local_coord local_coord;

                local_coord.coord.x = SCREEN_WIDTH / 2;
                local_coord.coord.y = SCREEN_HEIGHT / 2;

                vdp1_cmdt_local_coord_add(&local_coord);
                vdp1_cmdt_end();
        } vdp1_cmdt_list_end(0);

        struct vdp1_cmdt_polygon polygon;

        memset(&polygon, 0x00, sizeof(polygon));

        vdp2_tvmd_vblank_out_wait();
        vdp2_tvmd_vblank_in_wait();

        fix16_t angle = F16(0.0f);

        while (true) {
                vdp2_tvmd_vblank_out_wait();

                // Update
                matrix_stack_mode(MATRIX_STACK_MODE_MODEL_VIEW);
                matrix_stack_push(); {
                        matrix_stack_rotate(angle, 0);
                        matrix_stack_rotate(angle, 1);
                        matrix_stack_rotate(angle, 2);

                        angle = fix16_add(angle, F16(-1.0f));

                        model_polygon_project(teapot_vertices, teapot_indices,
                            teapot_normals, TEAPOT_POLYGON_CNT);
                } matrix_stack_pop();

                vdp1_cmdt_list_begin(1); {
                        int32_t idx;
                        for (idx = OT_PRIMITIVE_BUCKETS - 1; idx >= 0; idx--) {
                                /* Skip empty buckets */
                                if (ot_bucket_empty(idx)) {
                                        continue;
                                }

#if POLYGON_SORT == 1
                                ot_bucket_primitive_sort(idx & (OT_PRIMITIVE_BUCKETS - 1),
                                    OT_PRIMITIVE_BUCKET_SORT_INSERTION);
#endif

#if RENDER == 1
                                struct ot_primitive *otp;
                                TAILQ_FOREACH (otp, ot_bucket(idx), otp_entries) {
                                        polygon.color = otp->otp_color;
                                        polygon.draw_mode.transparent_pixel = true;
                                        polygon.draw_mode.end_code = true;

                                        polygon.vertex.a.x = otp->otp_coords[0].x;
                                        polygon.vertex.a.y = otp->otp_coords[0].y;
                                        polygon.vertex.b.x = otp->otp_coords[1].x;
                                        polygon.vertex.b.y = otp->otp_coords[1].y;
                                        polygon.vertex.c.x = otp->otp_coords[2].x;
                                        polygon.vertex.c.y = otp->otp_coords[2].y;
                                        polygon.vertex.d.x = otp->otp_coords[3].x;
                                        polygon.vertex.d.y = otp->otp_coords[3].y;

                                        vdp1_cmdt_polygon_add(&polygon);
                                }
#endif
                                /* Clear OT bucket */
                                ot_bucket_init(idx);
                        }
                        vdp1_cmdt_end();
                } vdp1_cmdt_list_end(1);

                vdp2_tvmd_vblank_in_wait();

                // Draw
                vdp1_cmdt_list_commit();
        }
}

static void
hardware_init(void)
{
        /* VDP2 */
        vdp2_init();

        /* VDP1 */
        vdp1_init();

        /* SMPC */
        smpc_init();
        smpc_peripheral_init();

        /* Disable interrupts */
        cpu_intc_disable();

        irq_mux_t *vblank_in;
        irq_mux_t *vblank_out;

        vblank_in = vdp2_tvmd_vblank_in_irq_get();
        irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);

        vblank_out = vdp2_tvmd_vblank_out_irq_get();
        irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);

        /* Enable interrupts */
        cpu_intc_enable();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR_4MBIT(2, 0x01FFFE),
            COLOR_RGB555(0, 0, 7));

        vdp2_tvmd_display_set();
}

static void
vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
}

static void
vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
        if ((vdp2_tvmd_vcount_get()) == 0) {
                tick = (tick & 0xFFFFFFFF) + 1;
        }
}

static void __unused
model_polygon_project(const fix16_vector4_t *vb, const uint32_t *ib,
    const fix16_vector4_t *nb, const uint32_t ib_cnt)
{
        fix16_vector4_t vertex_mv[4];
        fix16_vector4_t vertex_projected[4];

        fix16_matrix4_t *matrix_projection;
        matrix_projection = matrix_stack_top(
                MATRIX_STACK_MODE_PROJECTION)->ms_matrix;

        fix16_matrix4_t *matrix_model_view;
        matrix_model_view = matrix_stack_top(
                MATRIX_STACK_MODE_MODEL_VIEW)->ms_matrix;

        /* PVMv = (P(VM))v */

        /* Calculate world to object space matrix (inverse) */
        fix16_matrix4_t matrix_wo;
        fix16_matrix4_inverse(matrix_model_view, &matrix_wo);

        fix16_vector4_t view_forward;
        view_forward.x = F16(0.0f);
        view_forward.y = F16(0.0f);
        view_forward.z = F16(-1.0f);
        view_forward.w = F16(1.0f);

        fix16_vector4_t view_forward_object;
        fix16_vector4_matrix4_multiply(&matrix_wo, &view_forward,
            &view_forward_object);

        uint32_t idx;
        for (idx = 0; idx < (ib_cnt * 4); idx += 4) {
                uint16_t color;
                color = colors[idx >> 2];

                const fix16_vector4_t *vtx[4];

                /* Submit each vertex in this order: A, B, C, D */
                /* Vertex A */ vtx[0] = &vb[ib[idx]];
                /* Vertex B */ vtx[1] = &vb[ib[idx + 1]];
                /* Vertex C */ vtx[2] = &vb[ib[idx + 2]];
                /* Vertex D */ vtx[3] = &vb[ib[idx + 3]];

#if CULLING == 1
                /* Face culling */
                fix16_t dot;
                dot = fix16_vector4_dot(&nb[idx >> 2], &view_forward_object);
                if (dot < F16(0.0f)) {
                        continue;
                }
#endif

                fix16_vector4_matrix4_multiply(matrix_model_view, vtx[0],
                    &vertex_mv[0]);
                fix16_vector4_matrix4_multiply(matrix_model_view, vtx[1],
                    &vertex_mv[1]);
                fix16_vector4_matrix4_multiply(matrix_model_view, vtx[2],
                    &vertex_mv[2]);
                fix16_vector4_matrix4_multiply(matrix_model_view, vtx[3],
                    &vertex_mv[3]);

                fix16_vector4_matrix4_multiply(matrix_projection, &vertex_mv[0],
                    &vertex_projected[0]);
                fix16_vector4_matrix4_multiply(matrix_projection, &vertex_mv[1],
                    &vertex_projected[1]);
                fix16_vector4_matrix4_multiply(matrix_projection, &vertex_mv[2],
                    &vertex_projected[2]);
                fix16_vector4_matrix4_multiply(matrix_projection, &vertex_mv[3],
                    &vertex_projected[3]);

                ot_primitive_add(vertex_mv, vertex_projected, color);
        }
}
