/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdlib.h>

typedef struct {
        size_t user_total;
        size_t yaul_total;
} data_t;

static void
_walker(const mm_stats_walk_entry_t *walk_entry)
{
        data_t * const data = walk_entry->work;

        const char * const tag =
          (walk_entry->pool_type == MM_STATS_TYPE_USER) ? "user" : "yaul";

        if (walk_entry->used) {
                dbgio_printf("%*s: 0x%08X:0x%04X (%iB)\n",
                    4,
                    tag,
                    walk_entry->address,
                    walk_entry->size,
                    walk_entry->size);

                if (walk_entry->pool_type == MM_STATS_TYPE_USER) {
                    data->user_total += walk_entry->size;
                } else {
                    data->yaul_total += walk_entry->size;
                }
        }
}

int
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        malloc(128);
        malloc(256);
        malloc(512);

        data_t data = {
                .user_total = 0,
                .yaul_total = 0
        };

        mm_stats_walk(_walker, &data);

        dbgio_puts("\n");
        dbgio_printf("Total yaul: %10iB\n", data.yaul_total);
        dbgio_printf("Total user: %10iB\n", data.user_total);

        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();
}
