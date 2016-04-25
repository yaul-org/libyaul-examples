/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdbool.h>
#include <stdio.h>

#include "../engine.h"

struct smpc_peripheral_digital digital_pad;

uint32_t tick = 0;
uint32_t start_scanline = 0;
uint32_t end_scanline = 0;

char text_buffer[1024];

static void objects_update(void) __unused;
static void objects_draw(void) __unused;
static void objects_project(void) __unused;

static void object_project(const struct object *);

static void hardware_init(void);
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);

void
engine_init(void)
{
        hardware_init();
        cons_init(CONS_DRIVER_VDP2, 44, 30);

        /* Engine specific components */
        fs_init();
        objects_init();
        scene_init();
        matrix_stack_init();
}

void
engine_loop(void)
{
        while (true) {
                vdp2_tvmd_vblank_out_wait(); {
                        start_scanline = tick;
                        cons_buffer("[H[2J");
                        scene_handler_update();
                        objects_update();
                        /* physics_update(); */
                        objects_project();
                }

                vdp2_tvmd_vblank_in_wait(); {
                        end_scanline = tick;
                        scene_handler_draw();
                        objects_draw();
                        cons_flush();
                        vdp1_cmdt_list_commit();
                }
        }
}

static void
objects_update(void)
{
        const struct object **objects;
        objects = objects_list();

        uint32_t object_idx;
        for (object_idx = 0; objects[object_idx] != NULL; object_idx++) {
                const struct object *object;
                object = objects[object_idx];

                OBJECT_CALL_EVENT(object, update);

                /* Call its components */
                uint32_t component_idx;
                for (component_idx = 0;
                     component_idx < OBJECT(object, component_count);
                     component_idx++) {
                        struct component *component;
                        component = &OBJECT(object, component_list)[component_idx];
                        COMPONENT_CALL_EVENT(component, update);
                }
        }
}

static void
objects_draw(void)
{
        const struct object **objects;
        objects = objects_list();

        uint32_t object_idx;
        for (object_idx = 0; objects[object_idx] != NULL; object_idx++) {
                const struct object *object;
                object = objects[object_idx];

                bool visible;
                visible = OBJECT_COMPONENT(object, visible);

                if (!visible) {
                        continue;
                }

                OBJECT_CALL_EVENT(object, draw);

                /* Call the object's components */
                uint32_t component_idx;
                for (component_idx = 0;
                     component_idx < OBJECT(object, component_count);
                     component_idx++) {
                        struct component *component;
                        component = &OBJECT(object, component_list)[component_idx];
                        COMPONENT_CALL_EVENT(component, draw);
                }
        }
}

static void
objects_project(void)
{
        /* Look for the camera component in the objects list and adjust the
         * (inverse) view matrix */
        const struct camera *camera;

        camera = objects_component_camera_find();
        assert((camera != NULL) && "No camera found");

        fix16_matrix3_t matrix_view;
        fix16_matrix3_identity(&matrix_view);

        /* Manipulating the camera in world space amounts to
         * applying the inverse of transformations:
         *
         *     [R|t] = [R^T | -(R^T)t]
         *
         * Because all the affine transformations excluding
         * translations are prohibited, R=I, the inverse view
         * matrix is [I|-t]. */
        matrix_view.frow[0][2] = -OBJECT_COMPONENT(camera->object, transform).position.x;
        matrix_view.frow[1][2] = -OBJECT_COMPONENT(camera->object, transform).position.y;
        matrix_view.frow[2][2] = -OBJECT_COMPONENT(camera->object, transform).position.z;

        matrix_stack_mode(MATRIX_STACK_MODE_MODEL_VIEW);
        matrix_stack_load(&matrix_view);

        vdp1_cmdt_list_begin(0); {
                struct vdp1_cmdt_system_clip_coord system_clip;
                system_clip.scc_coord.x = camera->width - 1;
                system_clip.scc_coord.y = camera->height - 1;

                static struct vdp1_cmdt_user_clip_coord user_clip;
                user_clip.ucc_coords[0].x = 0;
                user_clip.ucc_coords[0].y = 0;
                user_clip.ucc_coords[1].x = camera->width - 1;
                user_clip.ucc_coords[1].y = camera->height - 1;

                struct vdp1_cmdt_local_coord local;
                local.lc_coord.x = 0;
                local.lc_coord.y = camera->width - 1;

                vdp1_cmdt_system_clip_coord_set(&system_clip);
                vdp1_cmdt_user_clip_coord_set(&user_clip);
                vdp1_cmdt_local_coord_set(&local);

                /* Draw in reversed order. Here we can take a shortcut and sort
                 * before projecting. */
                const struct object **objects;
                objects = objects_sorted_list();

                uint32_t object_idx;
                for (object_idx = 0; objects[object_idx] != NULL; object_idx++) {
                        const struct object *object;
                        object = objects[object_idx];

                        object_project(object);
                }

                vdp1_cmdt_end();
        } vdp1_cmdt_list_end(0);
}

static void
object_project(const struct object *object)
{
        const struct camera *camera;
        camera = objects_component_camera_find();
        assert((camera != NULL) && "No camera found");

        /* The camera should not be projected */
        if (camera->object == object) {
                return;
        }

        /* Only objects with a vertex list should be projected */
        if ((OBJECT_COMPONENT(object, vertex_list) == NULL) ||
            (OBJECT_COMPONENT(object, vertex_count) != 4)) {
                return;
        }

        matrix_stack_push(); {
                fix16_matrix3_t *matrix_model_view;
                matrix_stack_mode(MATRIX_STACK_MODE_MODEL_VIEW);

                matrix_stack_translate(
                        OBJECT_COMPONENT(object, transform).position.x,
                        OBJECT_COMPONENT(object, transform).position.y,
                        OBJECT_COMPONENT(object, transform).position.z);

                matrix_model_view = matrix_stack_top(
                    MATRIX_STACK_MODE_MODEL_VIEW)->ms_matrix;

                uint32_t i;

                /* Transform from object, world, view to clip space */

                /* For now, we'll only support 1 polygon */
                uint32_t vertex_count;
                vertex_count = OBJECT(object, vertex_count);

                fix16_vector3_t vertex_mv[4];
                for (i = 0; i < vertex_count; i++) {
                        const fix16_vector3_t *vertex;
                        vertex = &OBJECT_COMPONENT(object, vertex_list)[i];

                        fix16_vector3_matrix3_multiply(matrix_model_view,
                            vertex, &vertex_mv[i]);
                }

                /* No need to project :) */

                /* Clip space */

                /* From homogeneous clip space, no need to viewport
                 * space (NDC) */

                /* From viewport space (NDC) to screen space */
                int16_vector2_t screen_coords[4];
                for (i = 0; i < vertex_count; i++) {
                        int16_vector2_t *screen_coord;
                        screen_coord = &screen_coords[i];

                        screen_coord->x = fix16_to_int(vertex_mv[i].x);
                        screen_coord->y = fix16_to_int(vertex_mv[i].y);
                }

                int16_t width;
                width = screen_coords[3].x - screen_coords[0].x;
                int16_t height;
                height = screen_coords[1].y - screen_coords[0].y;

                struct vdp1_cmdt_local_coord local_coord;
                local_coord.lc_coord.x = (width / 2) + screen_coords[0].x;
                local_coord.lc_coord.y = camera->height - ((height / 2) + screen_coords[0].y);

                struct vdp1_cmdt_polygon polygon;
                polygon.cp_mode.raw = 0x0000;
                polygon.cp_mode.transparent_pixel = true;
                polygon.cp_mode.end_code = true;
                polygon.cp_color = COLOR_RGB_DATA | OBJECT_COMPONENT(object, color_list)[0];
                polygon.cp_grad = 0x00000000;
                polygon.cp_vertex.a.x = (width / 2) - 1;
                polygon.cp_vertex.a.y = -(height / 2);
                polygon.cp_vertex.b.x = (width / 2) - 1;
                polygon.cp_vertex.b.y = (height / 2) - 1;
                polygon.cp_vertex.c.x = -(width / 2);
                polygon.cp_vertex.c.y = (height / 2) - 1;
                polygon.cp_vertex.d.x = -(width / 2);
                polygon.cp_vertex.d.y = -(height / 2);

                vdp1_cmdt_local_coord_set(&local_coord);
                vdp1_cmdt_polygon_draw(&polygon);
        } matrix_stack_pop();
}

static void
hardware_init(void)
{
        /* VDP1 */
        vdp1_init();

        /* VDP2 */
        vdp2_init();
        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);
        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(2, 0x01FFFE), 0x9C00);

        vdp2_sprite_type_set(1);
        vdp2_sprite_type_priority_set(0, 7);
        vdp2_sprite_type_priority_set(1, 7);
        vdp2_sprite_type_priority_set(2, 7);
        vdp2_sprite_type_priority_set(3, 7);
        vdp2_sprite_type_priority_set(4, 7);
        vdp2_sprite_type_priority_set(5, 7);
        vdp2_sprite_type_priority_set(6, 7);
        vdp2_sprite_type_priority_set(7, 7);

        /* SMPC */
        smpc_init();
        smpc_peripheral_init();

        cpu_intc_disable(); {
                irq_mux_t *vblank_in;
                irq_mux_t *vblank_out;

                vblank_in = vdp2_tvmd_vblank_in_irq_get();
                irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);

                vblank_out = vdp2_tvmd_vblank_out_irq_get();
                irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);
        } cpu_intc_enable();
}

static void
vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        smpc_peripheral_digital_port(1, &digital_pad);
}

static void
vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
        if ((vdp2_tvmd_vcount_get()) == 0) {
                tick = (tick & 0xFFFFFFFF) + 1;
        }
}
