#include "mic3d.h"

#define SORT_LIST_SIZE   4
#define SORT_SINGLE_SIZE 16

point_t __pool_points[POINTS_COUNT] __aligned(16);

uint8_t __pool_sort_lists[SORT_DEPTH * SORT_LIST_SIZE] __aligned(16);
uint8_t __pool_sort_singles[POLYGON_COUNT * SORT_SINGLE_SIZE] __aligned(16);

vdp1_cmdt_t __pool_cmdts[POLYGON_COUNT] __aligned(16);
