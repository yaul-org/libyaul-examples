#include "state.h"

static texture_t _default_texture[] = {
        {
                .vram_index = TEXTURE_VRAM_INDEX(0),
                .size       = TEXTURE_SIZE(0, 0)
        }
};

void
__tlist_init(void)
{
        list_t * const tlist = &__state.tlist->list;

        tlist->flags = LIST_FLAGS_NONE;
        tlist->buffer = _default_texture;
        tlist->count = 0;
        tlist->size = sizeof(texture_t);
        tlist->default_element = _default_texture;
}

texture_t *
tlist_acquire(uint32_t texture_count)
{
        list_t * const list = &__state.tlist->list;

        __list_alloc(list, texture_count);

        return list->buffer;
}

void
tlist_release(void)
{
        list_t * const list = &__state.tlist->list;

        __list_free(list);
}

texture_t *
tlist_get(void)
{
        return __state.tlist->list.buffer;
}

void
tlist_set(texture_t *textures, uint16_t texture_count)
{
        list_t * const list = &__state.tlist->list;

        __list_set(list, textures, texture_count);
}
