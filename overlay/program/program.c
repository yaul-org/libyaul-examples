#include <sys/cdefs.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <yaul.h>

#define BACK_SCREEN VDP2_VRAM_ADDR(3, 0x1FFFE)

#define FILELIST_ENTRY_COUNT (16)

typedef int32_t (*overlay_start_t)(void *work);

static int32_t _overlay_exec(const char *overlay_filename, void *work);

static cdfs_filelist_t _filelist;

int
main(void)
{
        cdfs_filelist_entry_t * const filelist_entries =
            cdfs_entries_alloc(FILELIST_ENTRY_COUNT);
        assert(filelist_entries != NULL);

        cdfs_filelist_default_init(&_filelist, filelist_entries, FILELIST_ENTRY_COUNT);
        cdfs_filelist_root_read(&_filelist);

        int32_t overlay_ret;

        /* The overlay function signature can be changed. For this example, a
         * void pointer to work is used */
        uint32_t repeat_count = 3;
        overlay_ret = _overlay_exec("OVL2.BIN", &repeat_count);

        dbgio_puts("\n");
        dbgio_printf("Ret: %i. Back on main program\n", overlay_ret);
        dbgio_puts("\n");

        overlay_ret = _overlay_exec("OVL1.BIN", NULL);

        dbgio_puts("\n");
        dbgio_printf("Ret: %i. Back on main program\n", overlay_ret);
        dbgio_puts("\n");

        overlay_ret = _overlay_exec("OVL3.BIN", NULL);

        dbgio_puts("\n");
        dbgio_printf("Ret: %i. Back on main program\n", overlay_ret);
        dbgio_puts("Done\n");

        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();

        while (true) {
        }

        return 0;
}

void
user_init(void)
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
}

static int32_t
_overlay_exec(const char *overlay_filename, void *work)
{
        extern uintptr_t __overlay_start[];

        for (uint32_t i = 0; i < _filelist.entries_count; i++) {
                cdfs_filelist_entry_t * const file_entry = &_filelist.entries[i];

                if (*file_entry->name == '\0') {
                        continue;
                }

                if ((strcmp(file_entry->name, overlay_filename)) == 0) {
                        dbgio_printf("Loading \"%s\"...\n", overlay_filename);

                        int ret __unused;
                        ret = cd_block_sectors_read(file_entry->starting_fad, __overlay_start, file_entry->size);
                        assert(ret == 0);

                        overlay_start_t const overlay_start = (overlay_start_t)__overlay_start;

                        /* The cache needs to be purged as the overlay may make
                         * use of uncached variables/functions */

                        /* It's easier to just purge the overlay area, than it
                         * is to purge the overlay region, cache line by cache
                         * line */
                        cpu_cache_purge();

                        return overlay_start(work);
                }
        }

        return -1;
}
