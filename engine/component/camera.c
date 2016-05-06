#include "camera.h"

void
component_camera_on_init(struct component *this __unused)
{
}

void
component_camera_on_update(struct component *this __unused)
{
        cons_buffer("Hello from component camera\n");
}
