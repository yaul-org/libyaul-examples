/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "collider.h"
#include "objects.h"

static void object_show_init(struct object *) __unused;

void
component_collider_init(struct component *this)
{
        int16_t width;
        width = C_THIS(collider, width);

        int16_t height;
        height = C_THIS(collider, height);

        struct aabb *aabb;
        aabb = &C_THIS_P_DATA(collider, aabb);

        aabb->center.x = width / 2;
        aabb->center.y = height / 2;
        aabb->min.x = 0;
        aabb->min.y = 0;
        aabb->max.x = width - 1;
        aabb->max.y = height - 1;
        aabb->half.x = width / 2;
        aabb->half.y = height / 2;

        if (C_THIS(collider, show)) {
                struct object *object_show;
                object_show = &C_THIS_P_DATA(collider, show).object;

                fix16_vector3_t *vertex_list;
                vertex_list = &C_THIS_P_DATA(collider, show).vertex_list[0];

                vertex_list[0].x = F16(0.0f);
                vertex_list[0].y = F16(0.0f);
                vertex_list[0].z = F16(0.0f);

                vertex_list[1].x = F16(0.0f);
                vertex_list[1].y = F16(8.0f);
                vertex_list[1].z = F16(0.0f);

                vertex_list[2].x = F16(8.0f);
                vertex_list[2].y = F16(8.0f);
                vertex_list[2].z = F16(0.0f);

                vertex_list[3].x = F16(8.0f);
                vertex_list[3].y = F16(0.0f);
                vertex_list[3].z = F16(0.0f);

                color_rgb555_t *color_list;
                color_list = &C_THIS_P_DATA(collider, show).color_list[0];

                color_list[0].r = 31;
                color_list[0].g = 31;
                color_list[0].b = 31;

                OBJECT(object_show, active) = true;
                static int32_t x = 0;
                OBJECT(object_show, id) = OBJECT_ID_RESERVED_BEGIN + x;
                x++;
                OBJECT(object_show, visible) = true;
                OBJECT(object_show, vertex_list) = &C_THIS_P_DATA(collider, show).vertex_list[0];
                OBJECT(object_show, vertex_count) = 4;
                OBJECT(object_show, color_list) = &C_THIS_P_DATA(collider, show).color_list[0];

                OBJECT(object_show, transform).active = true;
                OBJECT(object_show, transform).object = (struct object *)object_show;
                OBJECT(object_show, transform).position.x = F16(0.0f);
                OBJECT(object_show, transform).position.y = F16(0.0f);
                OBJECT(object_show, transform).position.z = F16(1.0f);

                OBJECT(object_show, camera) = NULL;
                OBJECT(object_show, rigid_body) = NULL;
                OBJECT(object_show, colliders) = NULL;

                uint32_t component_idx;
                for (component_idx = 0;
                     component_idx < OBJECT_COMPONENT_LIST_MAX;
                     component_idx++) {
                        OBJECT(object_show, component_list)[component_idx] = NULL;
                }
                OBJECT(object_show, component_count) = 0;

                OBJECT(object_show, on_init) = NULL;
                OBJECT(object_show, on_update) = NULL;
                OBJECT(object_show, on_draw) = NULL;
                OBJECT(object_show, on_destroy) = NULL;
                OBJECT(object_show, on_collision) = NULL;
                OBJECT(object_show, on_trigger) = NULL;

                OBJECT(object_show, initialized) = false;
                OBJECT(object_show, context) = NULL;

                objects_object_child_add(
                        (struct object *)C_THIS(collider, object), object_show);

                // OBJECT_INIT(object_show);
        }
}

static void
object_show_init(struct object *this)
{
        assert(this != NULL);
}
