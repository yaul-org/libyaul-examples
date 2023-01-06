#ifndef MIC3D_H
#define MIC3D_H

#include "types.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 224

#define POLYGON_COUNT      4096
#define POINTS_COUNT       (POLYGON_COUNT * 4)
#define SORT_DEPTH         512
#define MATRIX_STACK_COUNT 16
#define CMDT_COUNT         2048

#define DEPTH_NEAR         20
#define DEPTH_FAR          256

#define TEXTURE_SIZE(w, h)       ((uint16_t)((((w) >> 3) << 8) | ((h) & 255)))
#define TEXTURE_VRAM_INDEX(addr) ((uint16_t)((uintptr_t)(addr) >> 3))

void mic3d_init(void);

void camera_lookat(const camera_t *camera);

void render_start(void);

void render_perspective_set(angle_t fov_angle);

void render_mesh_start(const mesh_t *mesh);
void render_mesh_transform(void);

void render_process(void);
void render(uint32_t cmdt_index);

texture_t *tlist_acquire(uint32_t texture_count);
void tlist_release(void);
void tlist_set(texture_t *textures, uint16_t texture_count);
texture_t *tlist_get(void);

void matrix_push(void);
void matrix_ptr_push(void);
void matrix_pop(void);
fix16_mat_t *matrix_top(void);
void matrix_copy(fix16_mat_t *m0);
void matrix_set(const fix16_mat_t *m0);
void matrix_x_translate(fix16_t x);
void matrix_y_translate(fix16_t y);
void matrix_z_translate(fix16_t z);
void matrix_translate(const fix16_vec3_t *t);
void matrix_translation_set(const fix16_vec3_t *t);
void matrix_translation_get(fix16_vec3_t *t);
void matrix_x_rotate(angle_t angle);
void matrix_y_rotate(angle_t angle);
void matrix_z_rotate(angle_t angle);

#endif /* MIC3D_H */
