#include "state.h"

extern fix16_vec3_t __pool_points[];
extern int16_vec2_t __pool_screen_points[];
extern fix16_t __pool_depth_values[];
extern polygon_meta_t __pool_polygons[];

extern sort_list_t __pool_sort_lists[];
extern sort_single_t __pool_sort_singles[];

extern vdp1_cmdt_t __pool_cmdts[];

static render_mesh_t _pool_render_meshes[16];

static render_t _render;
static sort_t _sort;

void
mic3d_init(void)
{
        __state.render = &_render;
        __state.sort = &_sort;

        _render.points_pool = __pool_points;
        _render.screen_points_pool = __pool_screen_points;
        _render.depth_values_pool = __pool_depth_values;
        _render.polygons_pool = __pool_polygons;
        _render.cmdts_pool = __pool_cmdts;
        _render.render_meshes_pool = _pool_render_meshes;

        _sort.singles_pool = __pool_sort_singles;
        _sort.sort_lists_pool = __pool_sort_lists;

        __render_init();
        __sort_init();
}
