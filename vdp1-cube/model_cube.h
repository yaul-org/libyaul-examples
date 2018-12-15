/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef MODEL_CUBE_H_
#define MODEL_CUBE_H_

#define CUBE_POLYGON_CNT 6

extern fix16_vector4_t cube_vertices[];
extern uint32_t cube_indices[CUBE_POLYGON_CNT * 4];
extern fix16_vector4_t cube_normals[CUBE_POLYGON_CNT];

#endif /* !MODEL_CUBE_H */
