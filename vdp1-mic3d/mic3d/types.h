#ifndef MIC3D_TYPES_H
#define MIC3D_TYPES_H

#include <fix16.h>
#include <vdp1.h>

typedef struct {
        fix16_t x;
        fix16_t y;
        fix16_t z;
} __aligned(4) point_t;

typedef struct {
        uint32_t p0;
        uint32_t p1;
        uint32_t p2;
        uint32_t p3;
}  polygon_t;

typedef struct {
        const point_t *points;
        uint32_t points_count;
        const polygon_t *polygons;
        uint32_t polygons_count;
} mesh_t;

typedef struct {
        point_t position;
} camera_t;

typedef int16_t angle_t;

#endif /* MIC3D_TYPES_H */
