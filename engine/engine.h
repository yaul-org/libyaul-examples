/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_H
#define ENGINE_H

#include <yaul.h>
#include <tga.h>

#ifdef HAVE_ASSERT
#include <assert.h>
#else
#undef assert
#define assert(...) do {                                                      \
} while(0)
#endif /* !HAVE_ASSERT */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/queue.h>

#include "aabb.h"
#include "bounding_box.h"
#include "fs.h"
#include "matrix_stack.h"
#include "scene.h"

#include "component.h"
#include "material.h"
#include "object.h"
#include "objects.h"
#include "physics.h"
#include "component/camera.h"
#include "component/collider.h"
#include "component/rigid_body.h"
#include "component/sprite.h"
#include "component/transform.h"

extern void engine_init(void);
extern void engine_loop(void);

extern uint32_t tick;
extern uint32_t start_scanline;
extern uint32_t end_scanline;

extern char text_buffer[];

extern struct smpc_peripheral_digital digital_pad;

#endif /* !ENGINE_H */
