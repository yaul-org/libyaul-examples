#ifndef MIC3D_LIGHT_H
#define MIC3D_LIGHT_H

#include <fix16.h>

#include "types.h"
#include "state.h"

typedef struct light {
        vdp1_gouraud_table_t *tables;
        uint32_t count;
        vdp1_vram_t vram_base;
        uint16_t slot_base;

        uint32_t used_count;
} __aligned(4) light_t;

void __light_init(void);

static inline uint16_t __always_inline
__light_slot_calculate(uint16_t slot)
{
        return (__state.light->slot_base + slot);
}

void __light_gst_put(void);

#endif /* MIC3D_LIGHT_H */
