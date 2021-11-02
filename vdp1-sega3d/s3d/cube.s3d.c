#include <sega3d.h>

static POINT point_cube1[] = {
        POStoFIXED(-20.0, -20.0,  20.0), /* 0 */
        POStoFIXED( 20.0, -20.0,  20.0), /* 1 */
        POStoFIXED( 20.0,  20.0,  20.0), /* 2 */
        POStoFIXED(-20.0,  20.0,  20.0), /* 3 */
        POStoFIXED(-20.0, -20.0, -20.0), /* 4 */
        POStoFIXED( 20.0, -20.0, -20.0), /* 5 */
        POStoFIXED( 20.0,  20.0, -20.0), /* 6 */
        POStoFIXED(-20.0,  20.0, -20.0)  /* 7 */
};

static POLYGON polygon_cube1[] = {
        NORMAL( 0.0,  0.0, 1.0), VERTICES(0, 1, 2, 3), /* Back */
        NORMAL(-1.0,  0.0, 0.0), VERTICES(4, 0, 3, 7), /* Left */
        NORMAL( 0.0,  0.0,-1.0), VERTICES(5, 4, 7, 6), /* Front */
        NORMAL( 1.0,  0.0, 0.0), VERTICES(1, 5, 6, 2), /* Right */
        NORMAL( 0.0, -1.0, 0.0), VERTICES(4, 5, 1, 0), /* Top */
        NORMAL( 0.0,  1.0, 0.0), VERTICES(3, 2, 6, 7)  /* Bottom */
};

static ATTR attribute_cube1[] = {
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB( 0,  0, 31), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB( 0, 31, 31), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB(31, 31, 31), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB(31,  0,  0), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB(31, 31,  0), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB(31,  0, 31), No_Gouraud, MESHoff, sprPolygon, No_Option),
};

XPDATA XDATA_S3D[] = {
        {
                point_cube1,
                sizeof(point_cube1) / sizeof(POINT),
                polygon_cube1,
                sizeof(polygon_cube1) / sizeof(POLYGON),
                attribute_cube1,
                NULL
        }
};

uint32_t XPDATA_S3D_COUNT = 1;
