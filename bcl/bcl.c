/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <bcl.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define NBG0_BPD    VDP2_VRAM_ADDR(0, 0x000000)

#define BACK_SCREEN VDP2_VRAM_ADDR(3, 0x01FFFE)

static cdfs_filelist_t _filelist;

int
main(void)
{
        cdfs_config_default_set();

        /* Load the maximum number. We have to free the allocated filelist
         * entries, but since we never exit, we don't have to */
        cdfs_filelist_entry_t * const filelist_entries =
            cdfs_entries_alloc(-1);
        assert(filelist_entries != NULL);

        cdfs_filelist_init(&_filelist, filelist_entries, -1);
        cdfs_filelist_root_read(&_filelist);

        uint32_t file_index;
        file_index = 0;

        while (true) {
                cdfs_filelist_entry_t *file_entry;

                for (; ; file_index++) {
                        if (file_index >= _filelist.entries_count) {
                                file_index = 0;
                        }

                        file_entry = &_filelist.entries[file_index];

                        if (file_entry->type == CDFS_ENTRY_TYPE_FILE) {
                                if ((strchr(file_entry->name, 'Z')) != NULL) {
                                        file_index++;
                                        break;
                                }
                        }
                }

                int ret __unused;
                ret = cd_block_sectors_read(file_entry->starting_fad, (void *)LWRAM(0x00000000), file_entry->size);
                assert(ret == 0);

                bcl_prs_decompress((void *)LWRAM(0x00000000), (void *)NBG0_BPD);

                vdp2_sync();
                vdp2_sync_wait();
        }

        return 0;
}

void
user_init(void)
{
        const vdp2_scrn_bitmap_format_t format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .ccc           = VDP2_SCRN_CCC_RGB_32768,
                .bitmap_size   = VDP2_SCRN_BITMAP_SIZE_512X256,
                .palette_base  = 0x00000000,
                .bitmap_base   = NBG0_BPD
        };

        cd_block_init();

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);

        const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[2].t0 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[3].t0 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS
        };

        vdp2_vram_cycp_set(&vram_cycp);

        vdp2_scrn_back_color_set(BACK_SCREEN,
            RGB1555(1, 5, 5, 7));

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();
}
