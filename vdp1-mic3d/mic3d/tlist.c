#include "state.h"

static texture_t _default_texture[] = {
        {
                .vram_index = 0,
                .size       = 0
        }
};

void
__tlist_init(void)
{
        list_t * const tlist = &__state.tlist->list;

        tlist->flags = LIST_FLAGS_NONE;
        tlist->list = _default_texture;
        tlist->count = 0;
        tlist->size = sizeof(texture_t);
        tlist->default_element = _default_texture;
}

texture_t *
tlist_alloc(uint32_t texture_count)
{
        list_t * const list = &__state.tlist->list;

        __list_alloc(list, texture_count);

        return list->list;
}

void
tlist_free(void)
{
        list_t * const list = &__state.tlist->list;

        __list_free(list);
}

void
tlist_set(texture_t *textures, uint16_t texture_count)
{
        list_t * const list = &__state.tlist->list;

        __list_set(list, textures, texture_count);
}
