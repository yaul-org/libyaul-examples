/*
 * 240p Test Suite for Nintendo 64
 * Copyright (C)2018 Artemio Urbina
 *
 * This file is part of the 240p Test Suite
 *
 * The 240p Test Suite is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The 240p Test Suite is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 240p Test Suite; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
 */

#include <yaul.h>

#include "fs.h"

extern uint8_t root_romdisk[];

static void *romdisk = NULL;

bool
fs_init(void)
{
        if (romdisk != NULL) {
                return false;
        }

        romdisk_init();
        romdisk = romdisk_mount("/", root_romdisk);

        if (romdisk == NULL) {
                return false;
        }

        return true;
}

void *
fs_open(const char *path)
{
        if (romdisk == NULL) {
                return NULL;
        }

        void *fh;
        fh = romdisk_open(romdisk, path);

        return fh;
}

void
fs_close(void *fh)
{
        if(romdisk == NULL) {
                return;
        }

        if (fh == NULL) {
                return;
        }

        romdisk_close(fh);
}

int32_t
fs_seek(void *fh, off_t seek, int whence)
{
        if (romdisk == NULL) {
                return -1;
        }

        if (fh == NULL) {
                return -1;
        }

        romdisk_seek(fh, seek, whence);

        return 0;
}

int32_t
fs_read(void *fh, void *dst, size_t len)
{
        if (romdisk == NULL) {
                return -1;
        }

        if (fh == NULL) {
                return -1;
        }

        if (len == 0) {
                return 0;
        }

        if (len > romdisk_total(fh)) {
                return -1;
        }

        size_t read_len;
        read_len = romdisk_read(fh, dst, len);

        return read_len;
}

bool
fs_read_whole(void *fh, void *dst)
{
        if (romdisk == NULL) {
                return false;
        }

        if (fh == NULL) {
                return false;
        }

        if (dst == NULL) {
                return false;
        }

        size_t read_len;
        read_len = romdisk_read(fh, dst, romdisk_total(fh));

        return (read_len == romdisk_total(fh));
}

size_t
fs_size(void *fh)
{
        if (romdisk == NULL) {
                return 0;
        }

        if (fh == NULL) {
                return 0;
        }

        return romdisk_total(fh);
}
