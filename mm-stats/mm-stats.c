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
        const char *tag;
        size_t total;
} data_t;

static void
_walker(const mm_stats_walk_entry_t *walk_entry)
{
        data_t * const data = walk_entry->work;

        if (walk_entry->used) {
                dbgio_printf("%*s: 0x%08X:0x%04X (%iB)\n",
                    7,
                    data->tag,
                    walk_entry->address,
                    walk_entry->size,
                    walk_entry->size);

                data->total += walk_entry->size;
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

        data_t private_data = {
                .tag   = "Private",
                .total = 0
        };

        data_t user_data = {
                .tag   = "User",
                .total = 0
        };

        mm_stats_yaul_walk(_walker, &private_data);
        mm_stats_walk(_walker, &user_data);

        dbgio_printf("Total private: %iB\n", private_data.total);
        dbgio_printf("Total user:    %iB\n", user_data.total);

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
