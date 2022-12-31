#include "state.h"

static render_t _render;
static sort_t _sort;
static tlist_t _tlist;

void
mic3d_init(void)
{
        __state.render = &_render;
        __state.sort = &_sort;
        __state.tlist = &_tlist;

        __render_init();
        __sort_init();
        __tlist_init();
}
