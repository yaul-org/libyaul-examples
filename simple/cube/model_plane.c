/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "cube.h"

fix16_vector4_t plane_vertices[] = {
        FIX16_VERTEX4(-0.25f,  0.25f, 0.0f, 1.0f),
        FIX16_VERTEX4( 0.25f,  0.25f, 0.0f, 1.0f),
        FIX16_VERTEX4( 0.25f, -0.25f, 0.0f, 1.0f),
        FIX16_VERTEX4(-0.25f, -0.25f, 0.0f, 1.0f),
};

uint32_t plane_indices[PLANE_POLYGON_CNT * 4] = {
        0, 1, 2, 3
};

fix16_vector4_t plane_normals[PLANE_POLYGON_CNT] = {
        FIX16_VERTEX4( 0.0f,  0.0f,  1.0f, 1.0f)
};
