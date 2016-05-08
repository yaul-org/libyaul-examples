/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "../engine.h"

extern uint8_t root_romdisk[];

static void *romdisk = NULL;

void
fs_init(void)
{
        romdisk_init();
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);
}

void *
fs_open(const char *path)
{
        assert(romdisk != NULL);

        void *fh;

        fh = romdisk_open(romdisk, path, O_RDONLY);
        assert(fh != NULL);

        return fh;
}

void
fs_close(void *fh)
{
        assert(romdisk != NULL);
        assert(fh != NULL);

        romdisk_close(fh);
}

void
fs_read(void *fh, void *dst, size_t len)
{
        assert(romdisk != NULL);
        assert(fh != NULL);

        size_t read_len __unused;
        read_len = romdisk_read(fh, dst, len);

        assert(read_len == len);
}

size_t
fs_size(void *fh)
{
        assert(romdisk != NULL);
        assert(fh != NULL);

        return romdisk_total(fh);
}

off_t
fs_seek(void *fh, off_t offset, int whence)
{
        assert(romdisk != NULL);
        assert(fh != NULL);

        return romdisk_seek(fh, offset, whence);
}

void
fs_texture_load(uint8_t type, const char *path, void *image_dst, void *cmap_dst)
{
        void *file_handle;
        file_handle = fs_open(path);

        size_t file_size;
        file_size = romdisk_total(file_handle);

        uint8_t *ptr;
        ptr = (uint8_t *)0x00201000;

        fs_read(file_handle, ptr, file_size);
        fs_close(file_handle);

        tga_t tga;
        int32_t status __unused;
        status = tga_read(&tga, ptr);
        assert(status == TGA_FILE_OK);

        int32_t amount __unused;
        amount = -1;

        switch (type) {
        case FS_LOAD_TEXTURE_1D:
                amount = tga_image_decode_tiled(&tga, image_dst);
                break;
        case FS_LOAD_TEXTURE_2D:
                amount = tga_image_decode(&tga, image_dst);
                break;
        }
        assert(amount >= 0);

        amount = tga_cmap_decode(&tga, cmap_dst);
        assert(amount >= 0);
}
