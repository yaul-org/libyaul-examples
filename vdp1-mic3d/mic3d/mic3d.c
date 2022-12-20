#include "state.h"

extern point_t __pool_points[];

extern uint8_t __pool_sort_lists[];
extern uint8_t __pool_sort_singles[];

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
        _render.cmdts_pool = __pool_cmdts;
        _render.render_meshes_pool = _pool_render_meshes;

        _sort.singles_pool = (sort_single_t *)__pool_sort_singles;
        _sort.sort_lists_pool = (sort_list_t *)__pool_sort_lists;

        __render_init();
        __sort_init();
}
