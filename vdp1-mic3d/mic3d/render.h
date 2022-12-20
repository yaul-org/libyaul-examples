#ifndef MIC3D_RENDER_H
#define MIC3D_RENDER_H

#include <fix16.h>

#include "types.h"

typedef struct {
        const mesh_t *mesh;
        const point_t *in_points;
        point_t *out_points;
        uint32_t polygons_count;
} render_mesh_t;

typedef struct {
        point_t *points_pool;
        vdp1_cmdt_t *cmdts_pool;
        render_mesh_t *render_meshes_pool;

        fix16_t view_distance;

        render_mesh_t *render_mesh_top;
        render_mesh_t *render_mesh;

        vdp1_cmdt_t *cmdts;
        uint32_t total_points_count;
        uint32_t total_polygons_count;
} render_t;

void __render_init(void);

#endif /* MIC3D_RENDER_H */
