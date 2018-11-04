/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef MODEL_PLANE_H_
#define MODEL_PLANE_H_

#define PLANE_POLYGON_CNT 1

extern fix16_vector4_t plane_vertices[];
extern uint32_t plane_indices[PLANE_POLYGON_CNT * 4];
extern fix16_vector4_t plane_normals[PLANE_POLYGON_CNT];

#endif /* !MODEL_PLANE_H */
