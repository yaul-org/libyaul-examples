#include <math.h>

#include "state.h"

static inline void __always_inline
_singles_reset(void)
{
        __state.sort->singles_top = __state.sort->singles_pool;
}

static inline sort_single_t * __always_inline
_singles_alloc(void)
{
        sort_single_t * const single = __state.sort->singles_top;

        __state.sort->singles_top++;

        return single;
}

void
__sort_init(void)
{
        _singles_reset();

        __state.sort->max_depth = 0;
}

void
__sort_start(void)
{
        __sort_init();
}

void
__sort_insert(const render_mesh_t *render_mesh, const  polygon_t *polygon, int32_t z)
{
        z = clamp(z, 0, SORT_DEPTH - 1);

        __state.sort->max_depth = max((uint32_t)z, __state.sort->max_depth);

        sort_single_t * const new_single = _singles_alloc();

        sort_list_t * const list_head = &__state.sort->sort_lists_pool[z];

        new_single->render_mesh = render_mesh;
        new_single->polygon = polygon;
        new_single->next_single = list_head->head;

        list_head->head = new_single;
}

void
__sort_iterate(sort_fn_t fn)
{
        sort_list_t *list_head;
        list_head = &__state.sort->sort_lists_pool[__state.sort->max_depth];

        for (uint32_t i = 0; i <= __state.sort->max_depth; i++, list_head--) {
                const sort_single_t *single;
                single = list_head->head;

                while (single != NULL) {
                        fn(single);

                        single = single->next_single;
                }

                list_head->head = NULL;
        }
}
