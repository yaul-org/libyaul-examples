/*
 * Copyright (c) 2006-2018
 * See LICENSE for details.
 *
 * Mic
 * Shazz
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   224

#define ELEMENT_COUNT(n) (sizeof((n)) / sizeof(*(n)))

#define INT2FIX(a) (((int32_t)(a))<<10)
#define FIX2INT(a) (((int32_t)(a))>>10)

typedef struct {
        int32_t x;
        int32_t y;
        int32_t z;
} __packed point;

typedef struct {
        int32_t p0;
        int32_t p1;
        int32_t p2;
        int32_t p3;
} __packed quad;

static void _hardware_init(void);

static uint8_t _sintb_buffer[] __aligned(4) = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x19, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
        0x00, 0x4B, 0x00, 0x00, 0x00, 0x64, 0x00,
        0x00, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x96,
        0x00, 0x00, 0x00, 0xAF, 0x00, 0x00, 0x00,
        0xC7, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00,
        0x00, 0xF8, 0x00, 0x00, 0x01, 0x11, 0x00,
        0x00, 0x01, 0x29, 0x00, 0x00, 0x01, 0x41,
        0x00, 0x00, 0x01, 0x58, 0x00, 0x00, 0x01,
        0x70, 0x00, 0x00, 0x01, 0x87, 0x00, 0x00,
        0x01, 0x9E, 0x00, 0x00, 0x01, 0xB5, 0x00,
        0x00, 0x01, 0xCC, 0x00, 0x00, 0x01, 0xE2,
        0x00, 0x00, 0x01, 0xF8, 0x00, 0x00, 0x02,
        0x0E, 0x00, 0x00, 0x02, 0x23, 0x00, 0x00,
        0x02, 0x38, 0x00, 0x00, 0x02, 0x4D, 0x00,
        0x00, 0x02, 0x61, 0x00, 0x00, 0x02, 0x75,
        0x00, 0x00, 0x02, 0x89, 0x00, 0x00, 0x02,
        0x9C, 0x00, 0x00, 0x02, 0xAF, 0x00, 0x00,
        0x02, 0xC2, 0x00, 0x00, 0x02, 0xD4, 0x00,
        0x00, 0x02, 0xE5, 0x00, 0x00, 0x02, 0xF6,
        0x00, 0x00, 0x03, 0x07, 0x00, 0x00, 0x03,
        0x17, 0x00, 0x00, 0x03, 0x27, 0x00, 0x00,
        0x03, 0x36, 0x00, 0x00, 0x03, 0x45, 0x00,
        0x00, 0x03, 0x53, 0x00, 0x00, 0x03, 0x61,
        0x00, 0x00, 0x03, 0x6E, 0x00, 0x00, 0x03,
        0x7A, 0x00, 0x00, 0x03, 0x87, 0x00, 0x00,
        0x03, 0x92, 0x00, 0x00, 0x03, 0x9D, 0x00,
        0x00, 0x03, 0xA8, 0x00, 0x00, 0x03, 0xB2,
        0x00, 0x00, 0x03, 0xBB, 0x00, 0x00, 0x03,
        0xC4, 0x00, 0x00, 0x03, 0xCC, 0x00, 0x00,
        0x03, 0xD3, 0x00, 0x00, 0x03, 0xDA, 0x00,
        0x00, 0x03, 0xE1, 0x00, 0x00, 0x03, 0xE7,
        0x00, 0x00, 0x03, 0xEC, 0x00, 0x00, 0x03,
        0xF0, 0x00, 0x00, 0x03, 0xF4, 0x00, 0x00,
        0x03, 0xF8, 0x00, 0x00, 0x03, 0xFB, 0x00,
        0x00, 0x03, 0xFD, 0x00, 0x00, 0x03, 0xFE,
        0x00, 0x00, 0x03, 0xFF, 0x00, 0x00, 0x04,
        0x00, 0x00, 0x00, 0x03, 0xFF, 0x00, 0x00,
        0x03, 0xFE, 0x00, 0x00, 0x03, 0xFD, 0x00,
        0x00, 0x03, 0xFB, 0x00, 0x00, 0x03, 0xF8,
        0x00, 0x00, 0x03, 0xF4, 0x00, 0x00, 0x03,
        0xF0, 0x00, 0x00, 0x03, 0xEC, 0x00, 0x00,
        0x03, 0xE7, 0x00, 0x00, 0x03, 0xE1, 0x00,
        0x00, 0x03, 0xDA, 0x00, 0x00, 0x03, 0xD3,
        0x00, 0x00, 0x03, 0xCC, 0x00, 0x00, 0x03,
        0xC4, 0x00, 0x00, 0x03, 0xBB, 0x00, 0x00,
        0x03, 0xB2, 0x00, 0x00, 0x03, 0xA8, 0x00,
        0x00, 0x03, 0x9D, 0x00, 0x00, 0x03, 0x92,
        0x00, 0x00, 0x03, 0x87, 0x00, 0x00, 0x03,
        0x7A, 0x00, 0x00, 0x03, 0x6E, 0x00, 0x00,
        0x03, 0x61, 0x00, 0x00, 0x03, 0x53, 0x00,
        0x00, 0x03, 0x45, 0x00, 0x00, 0x03, 0x36,
        0x00, 0x00, 0x03, 0x27, 0x00, 0x00, 0x03,
        0x17, 0x00, 0x00, 0x03, 0x07, 0x00, 0x00,
        0x02, 0xF6, 0x00, 0x00, 0x02, 0xE5, 0x00,
        0x00, 0x02, 0xD4, 0x00, 0x00, 0x02, 0xC2,
        0x00, 0x00, 0x02, 0xAF, 0x00, 0x00, 0x02,
        0x9C, 0x00, 0x00, 0x02, 0x89, 0x00, 0x00,
        0x02, 0x75, 0x00, 0x00, 0x02, 0x61, 0x00,
        0x00, 0x02, 0x4D, 0x00, 0x00, 0x02, 0x38,
        0x00, 0x00, 0x02, 0x23, 0x00, 0x00, 0x02,
        0x0E, 0x00, 0x00, 0x01, 0xF8, 0x00, 0x00,
        0x01, 0xE2, 0x00, 0x00, 0x01, 0xCC, 0x00,
        0x00, 0x01, 0xB5, 0x00, 0x00, 0x01, 0x9E,
        0x00, 0x00, 0x01, 0x87, 0x00, 0x00, 0x01,
        0x70, 0x00, 0x00, 0x01, 0x58, 0x00, 0x00,
        0x01, 0x41, 0x00, 0x00, 0x01, 0x29, 0x00,
        0x00, 0x01, 0x11, 0x00, 0x00, 0x00, 0xF8,
        0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00,
        0xC7, 0x00, 0x00, 0x00, 0xAF, 0x00, 0x00,
        0x00, 0x96, 0x00, 0x00, 0x00, 0x7D, 0x00,
        0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x4B,
        0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00,
        0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xE6, 0xFF, 0xFF, 0xFF, 0xCD, 0xFF,
        0xFF, 0xFF, 0xB4, 0xFF, 0xFF, 0xFF, 0x9B,
        0xFF, 0xFF, 0xFF, 0x82, 0xFF, 0xFF, 0xFF,
        0x69, 0xFF, 0xFF, 0xFF, 0x50, 0xFF, 0xFF,
        0xFF, 0x38, 0xFF, 0xFF, 0xFF, 0x1F, 0xFF,
        0xFF, 0xFF, 0x07, 0xFF, 0xFF, 0xFE, 0xEE,
        0xFF, 0xFF, 0xFE, 0xD6, 0xFF, 0xFF, 0xFE,
        0xBE, 0xFF, 0xFF, 0xFE, 0xA7, 0xFF, 0xFF,
        0xFE, 0x8F, 0xFF, 0xFF, 0xFE, 0x78, 0xFF,
        0xFF, 0xFE, 0x61, 0xFF, 0xFF, 0xFE, 0x4A,
        0xFF, 0xFF, 0xFE, 0x33, 0xFF, 0xFF, 0xFE,
        0x1D, 0xFF, 0xFF, 0xFE, 0x07, 0xFF, 0xFF,
        0xFD, 0xF1, 0xFF, 0xFF, 0xFD, 0xDC, 0xFF,
        0xFF, 0xFD, 0xC7, 0xFF, 0xFF, 0xFD, 0xB2,
        0xFF, 0xFF, 0xFD, 0x9E, 0xFF, 0xFF, 0xFD,
        0x8A, 0xFF, 0xFF, 0xFD, 0x76, 0xFF, 0xFF,
        0xFD, 0x63, 0xFF, 0xFF, 0xFD, 0x50, 0xFF,
        0xFF, 0xFD, 0x3D, 0xFF, 0xFF, 0xFD, 0x2B,
        0xFF, 0xFF, 0xFD, 0x1A, 0xFF, 0xFF, 0xFD,
        0x09, 0xFF, 0xFF, 0xFC, 0xF8, 0xFF, 0xFF,
        0xFC, 0xE8, 0xFF, 0xFF, 0xFC, 0xD8, 0xFF,
        0xFF, 0xFC, 0xC9, 0xFF, 0xFF, 0xFC, 0xBA,
        0xFF, 0xFF, 0xFC, 0xAC, 0xFF, 0xFF, 0xFC,
        0x9E, 0xFF, 0xFF, 0xFC, 0x91, 0xFF, 0xFF,
        0xFC, 0x85, 0xFF, 0xFF, 0xFC, 0x78, 0xFF,
        0xFF, 0xFC, 0x6D, 0xFF, 0xFF, 0xFC, 0x62,
        0xFF, 0xFF, 0xFC, 0x57, 0xFF, 0xFF, 0xFC,
        0x4D, 0xFF, 0xFF, 0xFC, 0x44, 0xFF, 0xFF,
        0xFC, 0x3B, 0xFF, 0xFF, 0xFC, 0x33, 0xFF,
        0xFF, 0xFC, 0x2C, 0xFF, 0xFF, 0xFC, 0x25,
        0xFF, 0xFF, 0xFC, 0x1E, 0xFF, 0xFF, 0xFC,
        0x18, 0xFF, 0xFF, 0xFC, 0x13, 0xFF, 0xFF,
        0xFC, 0x0F, 0xFF, 0xFF, 0xFC, 0x0B, 0xFF,
        0xFF, 0xFC, 0x07, 0xFF, 0xFF, 0xFC, 0x04,
        0xFF, 0xFF, 0xFC, 0x02, 0xFF, 0xFF, 0xFC,
        0x01, 0xFF, 0xFF, 0xFC, 0x00, 0xFF, 0xFF,
        0xFC, 0x00, 0xFF, 0xFF, 0xFC, 0x00, 0xFF,
        0xFF, 0xFC, 0x01, 0xFF, 0xFF, 0xFC, 0x02,
        0xFF, 0xFF, 0xFC, 0x04, 0xFF, 0xFF, 0xFC,
        0x07, 0xFF, 0xFF, 0xFC, 0x0B, 0xFF, 0xFF,
        0xFC, 0x0F, 0xFF, 0xFF, 0xFC, 0x13, 0xFF,
        0xFF, 0xFC, 0x18, 0xFF, 0xFF, 0xFC, 0x1E,
        0xFF, 0xFF, 0xFC, 0x25, 0xFF, 0xFF, 0xFC,
        0x2C, 0xFF, 0xFF, 0xFC, 0x33, 0xFF, 0xFF,
        0xFC, 0x3B, 0xFF, 0xFF, 0xFC, 0x44, 0xFF,
        0xFF, 0xFC, 0x4D, 0xFF, 0xFF, 0xFC, 0x57,
        0xFF, 0xFF, 0xFC, 0x62, 0xFF, 0xFF, 0xFC,
        0x6D, 0xFF, 0xFF, 0xFC, 0x78, 0xFF, 0xFF,
        0xFC, 0x85, 0xFF, 0xFF, 0xFC, 0x91, 0xFF,
        0xFF, 0xFC, 0x9E, 0xFF, 0xFF, 0xFC, 0xAC,
        0xFF, 0xFF, 0xFC, 0xBA, 0xFF, 0xFF, 0xFC,
        0xC9, 0xFF, 0xFF, 0xFC, 0xD8, 0xFF, 0xFF,
        0xFC, 0xE8, 0xFF, 0xFF, 0xFC, 0xF8, 0xFF,
        0xFF, 0xFD, 0x09, 0xFF, 0xFF, 0xFD, 0x1A,
        0xFF, 0xFF, 0xFD, 0x2B, 0xFF, 0xFF, 0xFD,
        0x3D, 0xFF, 0xFF, 0xFD, 0x50, 0xFF, 0xFF,
        0xFD, 0x63, 0xFF, 0xFF, 0xFD, 0x76, 0xFF,
        0xFF, 0xFD, 0x8A, 0xFF, 0xFF, 0xFD, 0x9E,
        0xFF, 0xFF, 0xFD, 0xB2, 0xFF, 0xFF, 0xFD,
        0xC7, 0xFF, 0xFF, 0xFD, 0xDC, 0xFF, 0xFF,
        0xFD, 0xF1, 0xFF, 0xFF, 0xFE, 0x07, 0xFF,
        0xFF, 0xFE, 0x1D, 0xFF, 0xFF, 0xFE, 0x33,
        0xFF, 0xFF, 0xFE, 0x4A, 0xFF, 0xFF, 0xFE,
        0x61, 0xFF, 0xFF, 0xFE, 0x78, 0xFF, 0xFF,
        0xFE, 0x8F, 0xFF, 0xFF, 0xFE, 0xA7, 0xFF,
        0xFF, 0xFE, 0xBE, 0xFF, 0xFF, 0xFE, 0xD6,
        0xFF, 0xFF, 0xFE, 0xEE, 0xFF, 0xFF, 0xFF,
        0x07, 0xFF, 0xFF, 0xFF, 0x1F, 0xFF, 0xFF,
        0xFF, 0x38, 0xFF, 0xFF, 0xFF, 0x50, 0xFF,
        0xFF, 0xFF, 0x69, 0xFF, 0xFF, 0xFF, 0x82,
        0xFF, 0xFF, 0xFF, 0x9B, 0xFF, 0xFF, 0xFF,
        0xB4, 0xFF, 0xFF, 0xFF, 0xCD, 0xFF, 0xFF,
        0xFF, 0xE6
};

static int32_t *_sintb = (int32_t *)&_sintb_buffer[0];

#define MODEL_POINT_COUNT       (ELEMENT_COUNT(_points_m) + ELEMENT_COUNT(_points_i) + ELEMENT_COUNT(_points_c))
#define MODEL_FACE_COUNT        (ELEMENT_COUNT(_face_m) + ELEMENT_COUNT(_face_i) + ELEMENT_COUNT(_face_c))

static point _points_m[28] = {
        {-5, -3, -2},
        {-3, -3, -2},
        { 3, -3, -2},
        { 5, -1, -2},
        { 5,  3, -2},
        { 3,  3, -2},
        { 3, -1, -2},
        { 1, -1, -2},
        { 1,  3, -2},
        {-1,  3, -2},
        {-1, -1, -2},
        {-3, -1, -2},
        {-3,  3, -2},
        {-5,  3, -2},
        //
        {-5, -3,  2},
        {-3, -3,  2},
        { 3, -3,  2},
        { 5, -1,  2},
        { 5,  3,  2},
        { 3,  3,  2},
        { 3, -1,  2},
        { 1, -1,  2},
        { 1,  3,  2},
        {-1,  3,  2},
        {-1, -1,  2},
        {-3, -1,  2},
        {-3,  3,  2},
        {-5,  3,  2}
};

static point _points_i[10] = {
        {-1, -3, -2},
        { 1, -1, -2},
        { 1,  3, -2},
        {-1,  3, -2},
        {-1, -1, -2},
        //
        {-1, -3,  2},
        { 1, -1,  2},
        { 1,  3,  2},
        {-1,  3,  2},
        {-1, -1,  2}
};

static point _points_c[22] = {
        {-3, -3, -2},
        { 1, -3, -2},
        { 3, -1, -2},
        { 1, -1, -2},
        {-1, -1, -2},
        {-1,  1, -2},
        { 3,  1, -2},
        { 3,  3, -2},
        {-1,  3, -2},
        {-3,  3, -2},
        {-3, -1, -2},
        //
        {-3, -3,  2},
        { 1, -3,  2},
        { 3, -1,  2},
        { 1, -1,  2},
        {-1, -1,  2},
        {-1,  1,  2},
        { 3,  1,  2},
        { 3,  3,  2},
        {-1,  3,  2},
        {-3,  3,  2},
        {-3, -1,  2}
};

static quad _face_m[23] = {
        { 0,  1, 12, 13},
        { 1,  2,  6, 11},
        { 6,  3,  4,  5},
        {10,  7,  8,  9},
        { 2,  3,  6,  6},
        {14, 15, 26, 27},
        {15, 16, 20, 25},
        {20, 17, 18, 19},
        {24, 21, 22, 23},
        {16, 17, 20, 20},
        { 0, 14, 16,  2},
        {14,  0, 13, 27},
        {13, 27, 26, 12},
        {11, 25, 26, 12},
        {25, 24, 10, 11},
        {24, 10,  9, 23},
        { 9, 23, 22,  8},
        { 7, 21, 22,  8},
        {21, 20,  6,  7},
        {20,  6,  5, 19},
        { 5, 19, 18,  4},
        { 3, 17, 18,  4},
        { 2, 16, 17,  3}
};

static quad _face_i[8] = {
        { 0,  1,  4,  4},
        { 1,  2,  3,  4},
        { 0,  5,  6,  1},
        { 1,  6,  7,  2},
        { 3,  8,  7,  2},
        { 5,  0,  3,  8},
        { 5,  6,  9,  9},
        { 6,  7,  8,  9}
};

static quad _face_c[16] = {
        { 0,  1,  3, 10},
        { 1,  2,  3,  3},
        {10,  4,  8,  9},
        { 5,  6,  7,  8},
        {11, 12, 14, 21},
        {12, 13, 14, 14},
        {21, 15, 19, 20},
        {16, 17, 18, 19},
        { 0, 11, 12,  1},
        { 1, 12, 13,  2},
        { 4, 15, 13,  2},
        { 4, 15, 16,  5},
        { 5, 16, 17,  6},
        { 6, 17, 18,  7},
        { 9, 20, 18,  7},
        {11,  0,  9, 20}
};

static point _rotated_m[ELEMENT_COUNT(_points_m)];
static point _projected_m[ELEMENT_COUNT(_points_m)];

static point _rotated_i[ELEMENT_COUNT(_points_i)];
static point _projected_i[ELEMENT_COUNT(_points_i)];

static point _rotated_c[ELEMENT_COUNT(_points_c)];
static point _projected_c[ELEMENT_COUNT(_points_c)];

/* Allow max 64 faces to be sorted */
static int32_t _avg_z[64];

static quad _faces[MODEL_FACE_COUNT];
static point _all_points[MODEL_POINT_COUNT];
static int32_t _face_order[MODEL_FACE_COUNT];

static point _camera;

static void _rotate(point *, point *, int32_t, int32_t);
static void _transform(point *, point *, int32_t, int32_t, int32_t, int32_t);
static void _project(point *, point *, int32_t);
static void _sort_quads(quad *, point *, int32_t *, int32_t);

static void _setup_drawing_env(struct vdp1_cmdt_list *, bool);
static void _setup_clear_fb(struct vdp1_cmdt_list *, const color_rgb555_t, bool);

void
main(void)
{
        _hardware_init();

        struct vdp1_cmdt_list *cmdt_lists[2];

        cmdt_lists[0] = vdp1_cmdt_list_alloc(5);
        cmdt_lists[1] = vdp1_cmdt_list_alloc(50);

        _setup_drawing_env(cmdt_lists[0], false);
        _setup_clear_fb(cmdt_lists[0], COLOR_RGB555(0, 15, 0), true);

        uint32_t i;
        uint32_t j;
        uint32_t k;

        for (i = 0; i < ELEMENT_COUNT(_points_m); i++) {
                _points_m[i].x = INT2FIX(_points_m[i].x * 6);
                _points_m[i].y = INT2FIX(_points_m[i].y * 6);
                _points_m[i].z = INT2FIX(_points_m[i].z * 6);
        }

        for (i = 0; i < ELEMENT_COUNT(_points_i); i++) {
                _points_i[i].x = INT2FIX(_points_i[i].x * 6);
                _points_i[i].y = INT2FIX(_points_i[i].y * 6);
                _points_i[i].z = INT2FIX(_points_i[i].z * 6);
        }

        for (i = 0; i < ELEMENT_COUNT(_points_c); i++) {
                _points_c[i].x = INT2FIX(_points_c[i].x * 6);
                _points_c[i].y = INT2FIX(_points_c[i].y * 6);
                _points_c[i].z = INT2FIX(_points_c[i].z * 6);
        }

        j = 0;

        for (i = 0; i < ELEMENT_COUNT(_face_m); i++, j++) {
                _faces[j].p0 = _face_m[i].p0;
                _faces[j].p1 = _face_m[i].p1;
                _faces[j].p2 = _face_m[i].p2;
                _faces[j].p3 = _face_m[i].p3;
        }

        for (i = 0; i < ELEMENT_COUNT(_face_i); i++, j++) {
                _faces[j].p0 = _face_i[i].p0 + 28;
                _faces[j].p1 = _face_i[i].p1 + 28;
                _faces[j].p2 = _face_i[i].p2 + 28;
                _faces[j].p3 = _face_i[i].p3 + 28;
        }

        for (i = 0; i < ELEMENT_COUNT(_face_c); i++, j++) {
                _faces[j].p0 = _face_c[i].p0 + 38;
                _faces[j].p1 = _face_c[i].p1 + 38;
                _faces[j].p2 = _face_c[i].p2 + 38;
                _faces[j].p3 = _face_c[i].p3 + 38;
        }

        _camera.x = 160;
        _camera.y = 112;
        _camera.z = -200;

        int32_t theta = 0;

        while (true) {
                vdp1_sync_draw(cmdt_lists[0]);

                _rotate(_points_m, _rotated_m, theta, 28);
                _rotate(_points_i, _rotated_i, theta, 10);
                _rotate(_points_c, _rotated_c, theta, 22);

                theta++;

                _transform(_rotated_m, _rotated_m, -50, 0, 0, 28);
                _transform(_rotated_c, _rotated_c, 35, 0, 0, 22);

                _project(_rotated_m, _projected_m, 28);
                _project(_rotated_i, _projected_i, 10);
                _project(_rotated_c, _projected_c, 22);

                j = 0;

                for (i = 0; i < ELEMENT_COUNT(_projected_m); i++, j++) {
                        _all_points[j] = _projected_m[i];
                }

                for (i = 0; i < ELEMENT_COUNT(_projected_i); i++, j++) {
                        _all_points[j] = _projected_i[i];
                }

                for (i = 0; i < ELEMENT_COUNT(_projected_c); i++, j++) {
                        _all_points[j] = _projected_c[i];
                }

                _sort_quads(_faces, _all_points, _face_order, MODEL_FACE_COUNT);

                vdp1_cmdt_list_reset(cmdt_lists[1]);

                for (i = 0; i < 47; i++) {
                        struct vdp1_cmdt_polygon polygon;

                        j = _face_order[i];

                        /* Check which direction this quad is facing */
                        k = ((-FIX2INT(_avg_z[i]) / 4) + 30);

                        uint8_t r;
                        r = 27 * k / 90 + 9;

                        uint8_t g;
                        g = 28 * k / 90 + 9;

                        uint8_t b;
                        b = 30 * k / 90 + 10;

                        color_rgb555_t color;
                        color = COLOR_RGB555(r, g, b);

                        polygon.cp_color = color;

                        polygon.cp_mode.raw = 0x0000;
                        polygon.cp_mode.pre_clipping = true;
                        polygon.cp_mode.mesh = false;

                        polygon.cp_vertex.a.x = _all_points[_faces[j].p0].x;
                        polygon.cp_vertex.a.y = _all_points[_faces[j].p0].y;
                        polygon.cp_vertex.b.x = _all_points[_faces[j].p3].x;
                        polygon.cp_vertex.b.y = _all_points[_faces[j].p3].y;
                        polygon.cp_vertex.c.x = _all_points[_faces[j].p2].x;
                        polygon.cp_vertex.c.y = _all_points[_faces[j].p2].y;
                        polygon.cp_vertex.d.x = _all_points[_faces[j].p1].x;
                        polygon.cp_vertex.d.y = _all_points[_faces[j].p1].y;

                        polygon.cp_grad = 0x00000000;

                        vdp1_cmdt_polygon_draw(cmdt_lists[1], &polygon);
                }

                vdp1_cmdt_end(cmdt_lists[1]);

                vdp1_sync_draw(cmdt_lists[1]);

                vdp_sync(0);
        }
}

static void
_rotate(point *in, point *out, int32_t angle, int32_t n)
{
        int32_t i;
        int32_t temp;
        int32_t can, san;

        san = _sintb[angle & 0xff];
        can = _sintb[(angle + 0x40) & 0xff];

        for (i = 0; i < n; i++) {
                /* About X */
                out[i].y = ((in[i].y * can) - (in[i].z * san));
                out[i].y = FIX2INT(out[i].y);
                out[i].z = ((in[i].y * san) + (in[i].z * can));
                out[i].z = FIX2INT(out[i].z);

                /* About Y */
                out[i].x = ((in[i].x * can) - (out[i].z * san));
                out[i].x = FIX2INT(out[i].x);
                out[i].z = ((in[i].x * san) + (out[i].z * can));
                out[i].z = FIX2INT(out[i].z);

                /* About Z */
                temp = out[i].x;
                out[i].x = ((out[i].x * can) - (out[i].y * san));
                out[i].x = FIX2INT(out[i].x);
                out[i].y = ((temp * san) + (out[i].y * can));
                out[i].y = FIX2INT(out[i].y);
        }

}

static void
_transform(point *in, point *out, int32_t xt, int32_t yt, int32_t zt, int32_t n)
{
        int32_t i;

        xt = INT2FIX(xt);
        yt = INT2FIX(yt);
        zt = INT2FIX(zt);

        for (i = 0; i < n; i++) {
                out[i].x = in[i].x + xt;
                out[i].y = in[i].y + yt;
                out[i].z = in[i].z + zt;
        }
}

static void
_project(point *in, point *out, int32_t n)
{
        int32_t i;

        for (i = 0; i < n; i++) {
                out[i].x = _camera.x + FIX2INT(((in[i].x * _camera.z) / (_camera.z - FIX2INT(in[i].z))));
                out[i].y = _camera.y + FIX2INT(((in[i].y * _camera.z) / (_camera.z - FIX2INT(in[i].z))));
                out[i].z = in[i].z;
        }
}

static void
_sort_quads(quad *f, point *p, int32_t *order, int32_t n)
{
        int32_t i;
        int32_t j;
        int32_t tmp;

        /* Initialize arrays */
        for (i = 0; i < n; i++) {
                _avg_z[i] = p[f[i].p0].z +
                            p[f[i].p1].z +
                            p[f[i].p2].z +
                            p[f[i].p3].z;
                order[i] = i;
        }


        /* Bubble-sort the whole lot.. yeehaw! */
        for (i = 0; i < n - 1; i++) {
                for (j = i + 1; j < n; j++) {
                        if (_avg_z[j] > _avg_z[i]) {
                                tmp = _avg_z[i];
                                _avg_z[i] = _avg_z[j];
                                _avg_z[j] = tmp;
                                tmp = order[i];
                                order[i] = order[j];
                                order[j] = tmp;
                        }
                }
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
                                  TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
                                        COLOR_RGB555(0, 3, 15));

        vdp2_sprite_priority_set(0, 6);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_setup_drawing_env(struct vdp1_cmdt_list *cmdt_list, bool end)
{
        struct vdp1_cmdt_local_coord local_coord = {
                .lc_coord = {
                        .x = 0,
                        .y = 0
                }
        };

        struct vdp1_cmdt_system_clip_coord system_clip = {
                .scc_coord = {
                        .x = SCREEN_WIDTH - 1,
                        .y = SCREEN_HEIGHT - 1
                }
        };

        struct vdp1_cmdt_user_clip_coord user_clip = {
                .ucc_coords = {
                        {
                                .x = 0,
                                .y = 0
                        },
                        {
                                .x = SCREEN_WIDTH - 1,
                                .y = SCREEN_HEIGHT - 1
                        }
                }
        };

        vdp1_cmdt_system_clip_coord_set(cmdt_list, &system_clip);
        vdp1_cmdt_user_clip_coord_set(cmdt_list, &user_clip);
        vdp1_cmdt_local_coord_set(cmdt_list, &local_coord);

        if (end) {
                vdp1_cmdt_end(cmdt_list);
        }
}

static void
_setup_clear_fb(struct vdp1_cmdt_list *cmdt_list, const color_rgb555_t color, bool end)
{
        struct vdp1_cmdt_polygon polygon;

        polygon.cp_mode.raw = 0x0000;
        polygon.cp_color = color;
        polygon.cp_vertex.a.x = 0;
        polygon.cp_vertex.a.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.b.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.b.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.c.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.c.y = 0;

        polygon.cp_vertex.d.x = 0;
        polygon.cp_vertex.d.y = 0;

        vdp1_cmdt_polygon_draw(cmdt_list, &polygon);

        if (end) {
                vdp1_cmdt_end(cmdt_list);
        }
}
