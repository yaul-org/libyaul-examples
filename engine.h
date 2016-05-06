/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_H
#define ENGINE_H

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>

#include "engine/aabb.h"
#include "engine/bounding_box.h"
#include "engine/component.h"
#include "engine/component/camera.h"
#include "engine/component/collider.h"
#include "engine/component/rigid_body.h"
#include "engine/component/sprite.h"
#include "engine/component/transform.h"
#include "engine/fs.h"
#include "engine/material.h"
#include "engine/matrix_stack.h"
#include "engine/object.h"
#include "engine/objects.h"
#include "engine/physics.h"
#include "engine/scene.h"

extern void engine_init(void);
extern void engine_loop(void);

extern uint32_t tick;
extern uint32_t start_scanline;
extern uint32_t end_scanline;

extern char text_buffer[];

extern struct smpc_peripheral_digital digital_pad;

#endif /* !ENGINE_H */
