#ifndef MIC3D_RENDER_H
#define MIC3D_RENDER_H

#include <fix16.h>

#include "types.h"

typedef enum {
        CLIP_FLAGS_NONE   = 0,
        CLIP_FLAGS_NEAR   = 1 << 0,
        CLIP_FLAGS_FAR    = 1 << 1,
        CLIP_FLAGS_LEFT   = 1 << 2,
        CLIP_FLAGS_RIGHT  = 1 << 3,
        CLIP_FLAGS_TOP    = 1 << 4,
        CLIP_FLAGS_BOTTOM = 1 << 5,
        CLIP_FLAGS_SIDE   = 1 << 6
} clip_flags_t;

typedef struct {
        uint16_t index;
        attribute_t attribute;
} __aligned(4) polygon_meta_t;

typedef struct {
        const mesh_t *mesh;

        const fix16_vec3_t *in_points;
        fix16_vec3_t *out_points;
        int16_vec2_t *screen_points;
        fix16_t *depth_values;

        const polygon_t *in_polygons;
        polygon_meta_t *out_polygons;

        uint32_t polygons_count;
} __aligned(4) render_mesh_t;

typedef struct {
        fix16_vec3_t *points_pool;
        int16_vec2_t *screen_points_pool;
        fix16_t *depth_values_pool;
        polygon_meta_t *polygons_pool;
        vdp1_cmdt_t *cmdts_pool;
        render_mesh_t *render_meshes_pool;

        fix16_t view_distance;
        fix16_t clip_factor;

        render_mesh_t *render_mesh_top;
        render_mesh_t *render_mesh;

        vdp1_cmdt_t *cmdts;
        uint32_t total_points_count;
        uint32_t total_polygons_count;
} __aligned(4) render_t;

void __render_init(void);

#endif /* MIC3D_RENDER_H */
