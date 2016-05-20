/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef ENGINE_FS_H
#define ENGINE_FS_H

#include "engine.h"

size_t fs_size(void *);
void *fs_open(const char *);
void fs_close(void *);
void fs_init(void);
void fs_read(void *, void *, size_t, size_t);
off_t fs_seek(void *, off_t, int);

#define FS_LOAD_TEXTURE_1D 0
#define FS_LOAD_TEXTURE_2D 1

void fs_texture_load(uint8_t, const char *, void *, void *);

#endif /* !ENGINE_FS_H */
