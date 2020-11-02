#include <sega3d.h>

static POINT point_cube1[] = {
        POStoFIXED(-20.0, -20.0,  20.0),
        POStoFIXED( 20.0, -20.0,  20.0),
        POStoFIXED( 20.0,  20.0,  20.0),
        POStoFIXED(-20.0,  20.0,  20.0),
        POStoFIXED(-20.0, -20.0, -20.0),
        POStoFIXED( 20.0, -20.0, -20.0),
        POStoFIXED( 20.0,  20.0, -20.0),
        POStoFIXED(-20.0,  20.0, -20.0)
};

static POLYGON polygon_cube1[] = {
        NORMAL( 0.0,  0.0, 1.0), VERTICES(0, 1, 2, 3),
        NORMAL(-1.0,  0.0, 0.0), VERTICES(4, 0, 3, 7),
        NORMAL( 0.0,  0.0,-1.0), VERTICES(5, 4, 7, 6),
        NORMAL( 1.0,  0.0, 0.0), VERTICES(1, 5, 6, 2),
        NORMAL( 0.0, -1.0, 0.0), VERTICES(4, 5, 1, 0),
        NORMAL( 0.0,  1.0, 0.0), VERTICES(3, 2, 6, 7)
};

static ATTR attribute_cube1[] = {
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB( 0,  0, 31), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB( 0, 31, 31), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB(31, 31, 31), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB(31,  0,  0), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB(31, 31,  0), No_Gouraud, MESHoff, sprPolygon, No_Option),
        ATTRIBUTE(Dual_Plane, SORT_CEN, No_Texture, C_RGB(31,  0, 31), No_Gouraud, MESHoff, sprPolygon, No_Option),
};

PDATA PD_CUBE1 = {
        point_cube1,
        sizeof(point_cube1) / sizeof(POINT), 
        polygon_cube1,
        sizeof(polygon_cube1) / sizeof(POLYGON), 
        attribute_cube1
};
