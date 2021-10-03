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

static iso9660_filelist_t _filelist;
static iso9660_filelist_entry_t _filelist_entries[ISO9660_FILELIST_ENTRIES_COUNT];

int
main(void)
{
        vdp2_tvmd_display_clear();

        vdp2_scrn_bitmap_format_t format;
        memset(&format, 0x00, sizeof(format));

        format.scroll_screen = VDP2_SCRN_NBG0;
        format.cc_count = VDP2_SCRN_CCC_RGB_32768;
        format.bitmap_size.width = 512;
        format.bitmap_size.height = 256;
        format.color_palette = 0x00000000;
        format.bitmap_pattern = VDP2_VRAM_ADDR(0, 0x00000);
        format.rp_mode = 0;
        format.sf_type = VDP2_SCRN_SF_TYPE_NONE;
        format.sf_code = VDP2_SCRN_SF_CODE_A;
        format.sf_mode = 0;

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* no_trans = */ false);

        vdp2_vram_cycp_t vram_cycp;

        vram_cycp.pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp.pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[2].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp.pt[3].t0 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t1 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp.pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_set(&vram_cycp);

        color_rgb1555_t bs_color;
        bs_color = COLOR_RGB1555(1, 5, 5, 7);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            bs_color);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);
        vdp2_tvmd_display_set();

        _filelist.entries = _filelist_entries;
        _filelist.entries_count = 0;
        _filelist.entries_pooled_count = 0;

        /* Load the maximum number */
        iso9660_filelist_root_read(&_filelist, -1);

        uint32_t i;
        i = 0;

        while (true) {
                iso9660_filelist_entry_t *file_entry;

                for (; ; i++) {
                        if (i >= _filelist.entries_count) {
                                i = 0;
                        }

                        file_entry = &_filelist.entries[i];

                        if (file_entry->type == ISO9660_ENTRY_TYPE_FILE) {
                                if ((strchr(file_entry->name, 'Z')) != NULL) {
                                        i++;
                                        break;
                                }
                        }
                }

                int ret __unused;
                ret = cd_block_sectors_read(file_entry->starting_fad, (void *)LWRAM(0x00000000), file_entry->size);
                assert(ret == 0);

                bcl_prs_decompress((void *)LWRAM(0x00000000), (void *)VDP2_VRAM_ADDR(0, 0x00000));

                vdp_sync();
        }

        return 0;
}
