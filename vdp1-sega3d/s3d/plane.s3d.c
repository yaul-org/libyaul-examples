#include <sega3d.h>

#define PN_SONIC        0
#define PN_AM2          1

static POINT point_PLANE1[] = {
        POStoFIXED(-10.0, -10.0, 0.0),
        POStoFIXED( 10.0, -10.0, 0.0),
        POStoFIXED( 10.0,  10.0, 0.0),
        POStoFIXED(-10.0,  10.0, 0.0),
};

static POLYGON polygon_PLANE1[] = {
        NORMAL(0.0, 1.0, 0.0), VERTICES(0, 1, 2, 3)
};

static ATTR attribute_PLANE1[] = {
        ATTRIBUTE(Dual_Plane, SORT_CEN, PN_AM2, No_Palet, No_Gouraud, CL32KRGB | MESHoff, sprNoflip, No_Option)
};

XPDATA XDATA_S3D[] = {
        {
                point_PLANE1,
                sizeof(point_PLANE1) / sizeof(POINT),
                polygon_PLANE1,
                sizeof(polygon_PLANE1) / sizeof(POLYGON),
                attribute_PLANE1,
                NULL
        }
};

uint32_t XPDATA_S3D_COUNT = 1;
