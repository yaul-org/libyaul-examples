#include "internal.h"

#include <fix16.h>

void
__light_init(void)
{
        light_set(NULL, 0, VDP1_VRAM(0x00000000));
}

void
__light_mesh_transform(void)
{
        /* XXX: Check if any lights are enabled */

        /* XXX: Check if the model needs lighting. Right now, each face is
         *      checked if the "use_lighting" flag is set */

        // XXX: Testing
        __state.light->color_matrix.frow[0][0] = FIX16(31);
        __state.light->color_matrix.frow[1][0] = FIX16(31);
        __state.light->color_matrix.frow[2][0] = FIX16(31);

        __state.light->color_matrix.frow[0][1] = FIX16(31);
        __state.light->color_matrix.frow[1][1] = FIX16( 0);
        __state.light->color_matrix.frow[2][1] = FIX16( 0);

        __state.light->color_matrix.frow[0][2] = FIX16(31);
        __state.light->color_matrix.frow[1][2] = FIX16( 0);
        __state.light->color_matrix.frow[2][2] = FIX16( 0);

        __state.light->light_matrix.row[0].x = FIX16_ZERO;
        __state.light->light_matrix.row[0].y = FIX16_ZERO;
        __state.light->light_matrix.row[0].z = FIX16_ONE;
        __state.light->light_enabled[0] = true;

        __state.light->light_matrix.row[1].x = FIX16_ZERO;
        __state.light->light_matrix.row[1].y = FIX16_ZERO;
        __state.light->light_matrix.row[1].z = FIX16_ZERO;
        __state.light->light_enabled[1] = false;

        __state.light->light_matrix.row[2].x = FIX16_ZERO;
        __state.light->light_matrix.row[2].y = FIX16_ZERO;
        __state.light->light_matrix.row[2].z = FIX16_ZERO;
        __state.light->light_enabled[2] = false;

        render_mesh_t * const render_mesh =
            __state.render->render_mesh;

        const fix16_mat43_t * const world_matrix = matrix_top();

        fix16_mat33_t inv_world_matrix __aligned(16);

        /* Invert here directly to a 3x3 matrix. If we use fix16_mat43_invert,
         * the translation vector is also inverted.
         *
         * The transpose also bakes the negation of the directional light
         * vector: f=dot(vn,-dir) */
        inv_world_matrix.frow[0][0] = -world_matrix->frow[0][0];
        inv_world_matrix.frow[0][1] = -world_matrix->frow[1][0];
        inv_world_matrix.frow[0][2] = -world_matrix->frow[2][0];

        inv_world_matrix.frow[1][0] = -world_matrix->frow[0][1];
        inv_world_matrix.frow[1][1] = -world_matrix->frow[1][1];
        inv_world_matrix.frow[1][2] = -world_matrix->frow[2][1];

        inv_world_matrix.frow[2][0] = -world_matrix->frow[0][2];
        inv_world_matrix.frow[2][1] = -world_matrix->frow[1][2];
        inv_world_matrix.frow[2][2] = -world_matrix->frow[2][2];

        fix16_mat33_t light_matrix __aligned(16);

        fix16_mat33_mul(&__state.light->light_matrix, &inv_world_matrix, &light_matrix);

        for (uint32_t i = 0; i < render_mesh->polygons_count; i++) {
                polygon_meta_t * const meta_polygon = &render_mesh->out_polygons[i];

                attribute_t * const attribute = &meta_polygon->attribute;

                if (!attribute->control.use_lighting) {
                        continue;
                }

                const gst_slot_t gst_slot = __light_gst_alloc();
                vdp1_gouraud_table_t * const gst = __light_gst_get(gst_slot);

                attribute->shading_slot =
                    __light_shading_slot_calculate(gst_slot);

                const polygon_t * const polygon =
                    &render_mesh->in_polygons[meta_polygon->index];

                for (uint32_t v = 0; v < 4; v++) {
                        const fix16_vec3_t * const vertex_normal =
                            &render_mesh->mesh->normals[polygon->p[v]];

                        fix16_t f[LIGHT_COUNT] = {
                                FIX16_ZERO,
                                FIX16_ZERO,
                                FIX16_ZERO
                        };

                        for (uint32_t i = 0; i < LIGHT_COUNT; i++) {
                                if (!__state.light->light_enabled[i]) {
                                        continue;
                                }

                                const fix16_vec3_t * const light_dir =
                                    &light_matrix.row[i];

                                f[i] = fix16_vec3_dot(vertex_normal, light_dir);
                        }

                        const fix16_vec3_t * const intensity =
                            (const fix16_vec3_t *)f;

                        fix16_vec3_t result;

                        fix16_mat33_vec3_mul(&__state.light->color_matrix, intensity, &result);

                        const uint16_t r = ((result.x >> 16) & 0x001F) + 16;
                        const uint16_t g = ((result.y >> 11) & 0x03E0) + 16;
                        const uint16_t b = ((result.z >>  6) & 0x7C00) + 16;

                        gst->colors[v].raw = 0x8000 | b | g | r;
                }
        }
}

void
__light_gst_put(void)
{
        if (__state.light->gst_count == 0) {
                return;
        }

        scu_dma_level_t level;
        level = scu_dma_level_unused_get();

        if (level < 0) {
                level = 0;
        }

        scu_dma_transfer(level, (void *)__state.light->vram_base,
            __state.light->gsts,
            __state.light->gst_count * sizeof(vdp1_gouraud_table_t));
        scu_dma_transfer_wait(level);

        __state.light->gst_count = 0;
}

void
light_set(vdp1_gouraud_table_t *gouraud_tables, uint32_t count, vdp1_vram_t vram_base)
{
        __state.light->gsts = gouraud_tables;
        __state.light->count = count;
        __state.light->vram_base = vram_base;
        __state.light->slot_base = vram_base >> 3;

        __state.light->gst_count = 0;
}

/* XXX: Rename */
void
light_direction_set(void)
{
        /* XXX: Testing */
        __state.light->gst_count = __state.light->count;
}
