#include "internal.h"

void
__light_init(void)
{
        light_set(NULL, 0, VDP1_VRAM(0x00000000));
}

void
__light_gst_put(void)
{
        if (__state.light->used_count == 0) {
                return;
        }

        scu_dma_level_t level;
        level = scu_dma_level_unused_get();

        if (level < 0) {
                level = 0;
        }

        scu_dma_transfer(level, (void *)__state.light->vram_base,
            __state.light->tables,
            __state.light->used_count * sizeof(vdp1_gouraud_table_t));
        scu_dma_transfer_wait(level);

        __state.light->used_count = 0;
}

void
light_set(vdp1_gouraud_table_t *gouraud_tables, uint32_t count, vdp1_vram_t vram_base)
{
        __state.light->tables = gouraud_tables;
        __state.light->count = count;
        __state.light->vram_base = vram_base;
        __state.light->slot_base = vram_base >> 3;

        __state.light->used_count = 0;
}

void
light_direction_set(void)
{
        /* XXX: Testing */
        __state.light->used_count = __state.light->count;
}
