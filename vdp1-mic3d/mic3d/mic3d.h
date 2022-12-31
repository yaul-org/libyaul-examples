#ifndef MIC3D_H
#define MIC3D_H

#include "types.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 224

#define POLYGON_COUNT 4096
#define POINTS_COUNT  (POLYGON_COUNT * 4)
#define SORT_DEPTH    256

#define DEPTH_NEAR    FIX16(20.0f)
#define DEPTH_FAR     FIX16(1024.0f)

#define RAD2ANGLE(d) ((angle_t)((65536.0f * (d)) / (2 * M_PI)))
#define DEG2ANGLE(d) ((angle_t)((65536.0f * (d)) / 360.0f))

void mic3d_init(void);

void render_start(void);

void render_perspective_set(angle_t fov_angle);

void render_mesh_start(const mesh_t *mesh);
void render_mesh_transform(const camera_t *camera);

void render_mesh_translate(fix16_t x, fix16_t y, fix16_t z);
void render_mesh_rotate_x(angle_t angle);
void render_mesh_rotate(fix16_t angle);

void render_process(void);
void render(uint32_t cmdt_index);

#endif /* MIC3D_H */
