/*
 * Copyright (c) 2012 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <tga.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

extern uint8_t root_romdisk[];

static void *_romdisk;

static vdp2_scrn_bitmap_format_t _format = {
        .scroll_screen = VDP2_SCRN_NBG0,
        .cc_count = VDP2_SCRN_CCC_RGB_32768,
        .bitmap_size.width = 512,
        .bitmap_size.height = 256,
        .color_palette = 0x00000000,
        .bitmap_pattern = VDP2_VRAM_ADDR(0, 0x00000),
        .rp_mode = 0,
        .sf_type = VDP2_SCRN_SF_TYPE_NONE,
        .sf_code = VDP2_SCRN_SF_CODE_A,
        .sf_mode = 0
};

/* Setting the no-access timing cycles from 0xF to 0xE increased performance
 * tremendously. This may be exasperated by the fact that we're writing to VDP2
 * VRAM outside of VBLANK-IN */
static const vdp2_vram_cycp_t _vram_cycp = {
        {
                {
                        .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t4 = VDP2_VRAM_CYCP_CPU_RW,
                        .t5 = VDP2_VRAM_CYCP_CPU_RW,
                        .t6 = VDP2_VRAM_CYCP_CPU_RW,
                        .t7 = VDP2_VRAM_CYCP_CPU_RW
                }, {
                        .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t4 = VDP2_VRAM_CYCP_CPU_RW,
                        .t5 = VDP2_VRAM_CYCP_CPU_RW,
                        .t6 = VDP2_VRAM_CYCP_CPU_RW,
                        .t7 = VDP2_VRAM_CYCP_CPU_RW
                }, {
                        .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t4 = VDP2_VRAM_CYCP_CPU_RW,
                        .t5 = VDP2_VRAM_CYCP_CPU_RW,
                        .t6 = VDP2_VRAM_CYCP_CPU_RW,
                        .t7 = VDP2_VRAM_CYCP_CPU_RW
                }, {
                        .t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                        .t4 = VDP2_VRAM_CYCP_CPU_RW,
                        .t5 = VDP2_VRAM_CYCP_CPU_RW,
                        .t6 = VDP2_VRAM_CYCP_CPU_RW,
                        .t7 = VDP2_VRAM_CYCP_CPU_RW
                }
        }
};

static const char *_tga_files[] = {
        "BITMAP1.TGA",
        "BITMAP2.TGA",
        "BITMAP3.TGA",
        "BITMAP4.TGA",
        "BITMAP5.TGA",
        "BITMAP6.TGA",
        "BITMAP7.TGA",
        "BITMAP8.TGA",
        "BITMAP9.TGA",
        NULL
};

uint8_t _intermediate_buffer[512 * 256 * 2] __aligned(4) __unused;

static void _hardware_init(void);
static void _romdisk_init(void);

static void _dma_handler(const dma_queue_transfer_t *transfer);

static uint32_t _tga_file_decode(const char *, void *, void *);

static void _dma_queue_enqueue(void *, void *, const uint32_t);

int
main(void)
{
        _hardware_init();
        _romdisk_init();

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* no_trans = */ false);

        vdp2_vram_cycp_set(&_vram_cycp);

        const char **current_file;
        current_file = &_tga_files[0];

        uint32_t bank;
        bank = 0;

        while (true) {
                if (*current_file == NULL) {
                        current_file = &_tga_files[0];
                }

                void *lwram;
                lwram = (void *)LWRAM(0x00000000);

                void *vram;
                vram = (void *)VDP2_VRAM_ADDR(2 * bank, 0x00000);

                uint32_t intermediate_size __unused;
                intermediate_size = _tga_file_decode(*current_file, lwram,
                    _intermediate_buffer);

                _dma_queue_enqueue(vram, _intermediate_buffer, intermediate_size);

                dma_queue_flush(DMA_QUEUE_TAG_IMMEDIATE);

                /* Set the back screen to its original color */
                vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(2 * bank, 0x01FFFE),
                    COLOR_RGB1555(1, 7, 7, 7));

                /* XXX: Hopefully one day, parts of the screen config can be
                 *      set, not the whole thing every time */
                _format.bitmap_pattern = VDP2_VRAM_ADDR(2 * bank, 0x00000);

                vdp2_scrn_bitmap_format_set(&_format);

                for (uint32_t i = 0; i < 15; i++) {
                        vdp2_tvmd_vblank_out_wait();
                        vdp2_tvmd_vblank_in_wait();
                }

                vdp2_sync_commit();

                dma_queue_flush_wait();

                current_file++;
                bank ^= 1;
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_clear();

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 6);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 7, 7, 7));

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_tvmd_display_set();

        vdp2_tvmd_vblank_out_wait();
        vdp2_tvmd_vblank_in_wait();        

        vdp2_sync_commit();
}

static void
_romdisk_init(void)
{
        romdisk_init();

        _romdisk = romdisk_mount("/", root_romdisk);
        assert(_romdisk != NULL);
}

static uint32_t
_tga_file_decode(const char *file, void *tga_buffer, void *buffer)
{
        assert(file != NULL);
        assert(tga_buffer != NULL);
        assert(buffer != NULL);

        void *fh;

        fh = romdisk_open(_romdisk, file);
        assert(fh != NULL);

        tga_t tga;
        int ret;
        size_t len;

        len = romdisk_read(fh, tga_buffer, romdisk_total(fh));
        assert(len == romdisk_total(fh));

        ret = tga_read(&tga, tga_buffer);
        assert(ret == TGA_FILE_OK);

        romdisk_close(fh);

        /* XXX: The TGA library isn't very well written. The pixel count is
         *      return, but because we're dealing with 16-BPP TGA images, it's 2
         *      bytes per pixel */
        return 2 * tga_image_decode(&tga, buffer);
}

static void
_dma_queue_enqueue(void *dst, void *src, const uint32_t size)
{
        static struct scu_dma_level_cfg dma_cfg = {
                .mode = SCU_DMA_MODE_DIRECT,
                .stride = SCU_DMA_STRIDE_2_BYTES,
                .update = SCU_DMA_UPDATE_NONE
        };

        dma_cfg.xfer.direct.len = size;
        dma_cfg.xfer.direct.dst = (uint32_t)dst;
        dma_cfg.xfer.direct.src = CPU_CACHE_THROUGH | (uint32_t)src;

        scu_dma_handle_t dma_handle;

        scu_dma_config_buffer(&dma_handle, &dma_cfg);

        dma_queue_enqueue(&dma_handle, DMA_QUEUE_TAG_IMMEDIATE, _dma_handler, NULL);
}

static void
_dma_handler(const dma_queue_transfer_t *transfer)
{
        assert(transfer->status == DMA_QUEUE_STATUS_COMPLETE);
}
