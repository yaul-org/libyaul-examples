#include "mic3d.h"

#define POStoFIXED(x, y, z)  {FIX16(x), FIX16(y), FIX16(z)}
#define NORMAL(x, y, z)
#define VERTICES(a, b, c, d) {.p0 = a, .p1 = b, .p2 = c, .p3 = d}

static const point_t point_cube1[] = {
        POStoFIXED(-10.0, -10.0,  10.0), /* 0 */
        POStoFIXED( 10.0, -10.0,  10.0), /* 1 */
        POStoFIXED( 10.0,  10.0,  10.0), /* 2 */
        POStoFIXED(-10.0,  10.0,  10.0), /* 3 */
        POStoFIXED(-10.0, -10.0, -10.0), /* 4 */
        POStoFIXED( 10.0, -10.0, -10.0), /* 5 */
        POStoFIXED( 10.0,  10.0, -10.0), /* 6 */
        POStoFIXED(-10.0,  10.0, -10.0)  /* 7 */
};

static const polygon_t polygon_cube1[] = {
        VERTICES(0, 1, 2, 3), /* Back */
        VERTICES(4, 0, 3, 7), /* Left */
        VERTICES(5, 4, 7, 6), /* Front */
        VERTICES(1, 5, 6, 2), /* Right */
        VERTICES(4, 5, 1, 0), /* Top */
        VERTICES(3, 2, 6, 7)  /* Bottom */
};

const mesh_t mesh_cube = {
        .points         = point_cube1,
        .points_count   = 8,
        .polygons       = polygon_cube1,
        .polygons_count = 6
};
