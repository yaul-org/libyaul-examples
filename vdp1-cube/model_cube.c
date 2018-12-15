/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "cube.h"

fix16_vector4_t cube_vertices[] = {
        FIX16_VERTEX4(-0.25f,  0.25f,  0.25f, 1.0f),
        FIX16_VERTEX4( 0.25f,  0.25f,  0.25f, 1.0f),
        FIX16_VERTEX4( 0.25f, -0.25f,  0.25f, 1.0f),
        FIX16_VERTEX4(-0.25f, -0.25f,  0.25f, 1.0f),

        FIX16_VERTEX4(-0.25f,  0.25f, -0.25f, 1.0f),
        FIX16_VERTEX4( 0.25f,  0.25f, -0.25f, 1.0f),
        FIX16_VERTEX4( 0.25f, -0.25f, -0.25f, 1.0f),
        FIX16_VERTEX4(-0.25f, -0.25f, -0.25f, 1.0f)
};

uint32_t cube_indices[CUBE_POLYGON_CNT * 4] = {
        /* Front */
        0, 1, 2, 3,
        /* Back */
        5, 4, 7, 6,
        /* Left */
        4, 0, 3, 7,
        /* Right */
        1, 5, 6, 2,
        /* Top */
        0, 4, 5, 1,
        /* Bottom */
        7, 3, 2, 6
};

fix16_vector4_t cube_normals[CUBE_POLYGON_CNT] = {
        FIX16_VERTEX4( 0.0f,  0.0f,  1.0f, 1.0f),
        FIX16_VERTEX4( 0.0f,  0.0f, -1.0f, 1.0f),
        FIX16_VERTEX4(-1.0f,  0.0f,  0.0f, 1.0f),
        FIX16_VERTEX4( 1.0f,  0.0f,  0.0f, 1.0f),
        FIX16_VERTEX4( 0.0f,  1.0f,  0.0f, 1.0f),
        FIX16_VERTEX4( 0.0f, -1.0f,  0.0f, 1.0f)
};
