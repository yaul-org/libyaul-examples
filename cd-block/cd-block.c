/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdlib.h>

#include "menu.h"

static void _vblank_out_handler(void *);

static void _hardware_init(void);

static void _walk(const iso9660_filelist_entry_t *, void *);

static smpc_peripheral_digital_t _digital;

int
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        iso9660_filelist_walk(_walk, NULL);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                dbgio_flush();
                vdp_sync();
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();

        int ret;
        ret = cd_block_init(0x0002);
        assert(ret == 0);

        if ((cd_block_cmd_is_auth(NULL)) == 0) {
                ret = cd_block_bypass_copy_protection();
                assert(ret == 0);
        }
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_walk(const iso9660_filelist_entry_t *entry, void *args __unused)
{
        dbgio_printf("%s ", entry->name);
}
