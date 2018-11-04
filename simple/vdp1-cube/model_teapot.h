/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef MODEL_TEAPOT_H_
#define MODEL_TEAPOT_H_

#define TEAPOT_POLYGON_CNT 514

extern fix16_vector4_t teapot_vertices[];
extern uint32_t teapot_indices[TEAPOT_POLYGON_CNT * 4];
extern fix16_vector4_t teapot_normals[TEAPOT_POLYGON_CNT];

#endif /* !MODEL_TEAPOT_H */
