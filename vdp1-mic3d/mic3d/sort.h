#ifndef MIC3D_SORT_H
#define MIC3D_SORT_H

#include <assert.h>
#include <stdint.h>

#include "render.h"

typedef struct sort_single {
        const render_mesh_t *render_mesh;
        const polygon_meta_t *polygon;
        struct sort_single *next_single;
        unsigned int :32;
} __aligned(4) sort_single_t;

static_assert(sizeof(sort_single_t) == 16);

typedef struct {
        sort_single_t *head;
} __aligned(4) sort_list_t;

static_assert(sizeof(sort_list_t) == 4);

typedef struct sort {
        sort_single_t *singles_pool;
        sort_list_t *sort_lists_pool;

        sort_single_t *singles_top;
        uint32_t max_depth;
} sort_t;

typedef void (*sort_fn_t)(const sort_single_t *single);

void __sort_init(void);
void __sort_start(void);
void __sort_insert(const render_mesh_t *render_mesh, const polygon_meta_t *polygon, int32_t z);
void __sort_iterate(sort_fn_t fn);

#endif /* MIC3D_SORT_H */
