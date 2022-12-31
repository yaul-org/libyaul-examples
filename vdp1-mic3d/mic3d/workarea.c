#include "state.h"

#define POOL_POINTS_COUNT       POINTS_COUNT
#define POOL_POLYGONS_COUNT     POLYGON_COUNT
#define POOL_SORT_LIST_COUNT    SORT_DEPTH
#define POOL_SORT_SINGLES_COUNT POLYGON_COUNT
#define POOL_CMDTS_COUNT        POLYGON_COUNT

point_t __pool_points[POOL_POINTS_COUNT] __aligned(16);
int16_vec2_t __pool_screen_points[POOL_POINTS_COUNT] __aligned(16);

polygon_meta_t __pool_polygons[POOL_POLYGONS_COUNT] __aligned(16);

sort_list_t __pool_sort_lists[POOL_SORT_LIST_COUNT] __aligned(16);
sort_single_t __pool_sort_singles[POOL_SORT_SINGLES_COUNT] __aligned(16);

vdp1_cmdt_t __pool_cmdts[POOL_CMDTS_COUNT] __aligned(16);
