#include <sys/cdefs.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <yaul.h>

#define BACK_SCREEN VDP2_VRAM_ADDR(3, 0x1FFFE)

extern uintptr_t _overlay_start[];

typedef int32_t (*overlay_start_t)(void *work);

static int32_t _overlay_exec(const char *overlay_filename, void *work);

static cdfs_filelist_t _filelist;

int
main(void)
{
        cd_block_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);
        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 0, 7, 0));

        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();

        /* Load the maximum number. We have to free the allocated filelist
         * entries, but since we never exit, we don't have to */
        cdfs_filelist_entry_t * const filelist_entries =
            cdfs_entries_alloc(16);
        assert(filelist_entries != NULL);

        cdfs_filelist_default_init(&_filelist, filelist_entries, 16);
        cdfs_filelist_root_read(&_filelist);

        /* Call OVL2.BIN to repeat a message */
        uint32_t repeat_times = 3;
        const int32_t overlay_ret = _overlay_exec("OVL2.BIN", &repeat_times);
        dbgio_printf("overlay_ret: %i\n", overlay_ret);

        dbgio_puts("\n");
        dbgio_printf("Back on main prog\n");
        dbgio_puts("\n");

        _overlay_exec("OVL1.BIN", NULL);

        dbgio_puts("\n");
        dbgio_printf("Back on main prog\n");
        dbgio_puts("\n");

        _overlay_exec("OVL3.BIN", NULL);

        dbgio_puts("\n");
        dbgio_printf("Back on main prog\n");
        dbgio_puts("Done\n");

        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }

        return 0;
}

static int32_t
_overlay_exec(const char *overlay_filename, void *work)
{
        for (uint32_t i = 0; i < _filelist.entries_count; i++) {
                cdfs_filelist_entry_t * const file_entry = &_filelist.entries[i];

                if (*file_entry->name == '\0') {
                        continue;
                }

                if ((strcmp(file_entry->name, overlay_filename)) == 0) {
                        dbgio_printf("Loading \"%s\"...\n", overlay_filename);

                        int ret __unused;
                        ret = cd_block_sectors_read(file_entry->starting_fad, _overlay_start, file_entry->size);
                        assert(ret == 0);

                        overlay_start_t const overlay_start = (overlay_start_t)_overlay_start;

                        cpu_cache_purge();

                        return overlay_start(work);
                }
        }

        return -1;
}
