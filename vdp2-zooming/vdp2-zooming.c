/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 * shazz <shazz@trsi.de>
 */

#include <yaul.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define NBG1_CPD                VDP2_VRAM_ADDR_4MBIT(2, 0x00000)
#define NBG1_PAL                VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define NBG1_MAP_PLANE_A        VDP2_VRAM_ADDR_4MBIT(0, 0x00000)
#define NBG1_MAP_PLANE_B        VDP2_VRAM_ADDR_4MBIT(0, 0x08000)
#define NBG1_MAP_PLANE_C        VDP2_VRAM_ADDR_4MBIT(0, 0x10000)
#define NBG1_MAP_PLANE_D        VDP2_VRAM_ADDR_4MBIT(0, 0x18000)

static void _hardware_init(void);

static void _transfer_cpd(void);
static void _transfer_pal(void);
static void _transfer_pnd(const struct vdp2_scrn_cell_format *);

static void _fill_map_pnd(uint16_t *, uint16_t, uint16_t, uint16_t);

static const color_rgb555_t _palette[] __unused = {
        COLOR_RGB888_RGB555(  0,   0,   0),
        COLOR_RGB888_RGB555(  0,   0, 170),
        COLOR_RGB888_RGB555(  0, 170,   0),
        COLOR_RGB888_RGB555(  0, 170, 170),
        COLOR_RGB888_RGB555( 85,  85,  85),
        COLOR_RGB888_RGB555( 85,  85, 255),
        COLOR_RGB888_RGB555( 85, 255,  85),
        COLOR_RGB888_RGB555( 85, 255, 255),
        COLOR_RGB888_RGB555(170,   0,   0),
        COLOR_RGB888_RGB555(170,   0, 170),
        COLOR_RGB888_RGB555(170,  85,   0),
        COLOR_RGB888_RGB555(170, 170, 170),
        COLOR_RGB888_RGB555(255,  85,  85),
        COLOR_RGB888_RGB555(255,  85, 255),
        COLOR_RGB888_RGB555(255, 255,  85),
        COLOR_RGB888_RGB555(255, 255, 255)
};

int
main(void)
{
        _hardware_init();

        fix16_t scroll_x;
        scroll_x = F16(0.0f);

        fix16_t scroll_y;
        scroll_y = F16(0.0f);

        q0_3_8_t zoom;
        zoom = VDP2_SCRN_REDUCTION_MIN;

        int32_t zoom_dir;
        zoom_dir = 1;

        while (true) {
                vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG1, scroll_x);
                vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG1, scroll_y);

                vdp2_scrn_reduction_x_set(VDP2_SCRN_NBG1, zoom);
                vdp2_scrn_reduction_y_set(VDP2_SCRN_NBG1, zoom);

                scroll_x = fix16_add(scroll_x, F16(16.0f));
                scroll_y = fix16_add(scroll_y, F16(16.0f));

                zoom = zoom + (zoom_dir * Q0_3_8(0.125f));

                if (zoom >= VDP2_SCRN_REDUCTION_MAX) {
                        zoom = VDP2_SCRN_REDUCTION_MAX;
                        zoom_dir = -1;
                } else if (zoom <= VDP2_SCRN_REDUCTION_MIN) {
                        zoom = VDP2_SCRN_REDUCTION_MIN;
                        zoom_dir = 1;
                }

                vdp_sync(0);
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_clear();

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR_4MBIT(2, 0x01FFFE),
            COLOR_RGB555(0, 0, 7));

        const struct vdp2_scrn_cell_format format = {
                .scf_scroll_screen = VDP2_SCRN_NBG1,
                .scf_cc_count = VDP2_SCRN_CCC_PALETTE_16,
                .scf_character_size = 1 * 1,
                .scf_pnd_size = 1,
                .scf_auxiliary_mode = 1,
                .scf_plane_size = 2 * 2,
                .scf_cp_table = NBG1_CPD,
                .scf_color_palette = NBG1_PAL,
                .scf_map = {
                        .planes = {
                                NBG1_MAP_PLANE_A,
                                NBG1_MAP_PLANE_B,
                                NBG1_MAP_PLANE_C,
                                NBG1_MAP_PLANE_D
                        }
                }
        };

        vdp2_scrn_cell_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG1, 6);
        vdp2_scrn_display_set(VDP2_SCRN_NBG1, /* transparent = */ false);

        struct vdp2_vram_cycp_bank vram_cycp_bank[2];

        vram_cycp_bank[0].t0 = VDP2_VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank[0].t1 = VDP2_VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank[0].t2 = VDP2_VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank[0].t3 = VDP2_VRAM_CYCP_PNDR_NBG1;
        vram_cycp_bank[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp_bank[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG1;
        vram_cycp_bank[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vdp2_vram_cycp_bank_set(0, &vram_cycp_bank[0]);
        vdp2_vram_cycp_bank_clear(1);
        vdp2_vram_cycp_bank_set(2, &vram_cycp_bank[1]);
        vdp2_vram_cycp_bank_clear(3);

        vdp2_scrn_reduction_set(VDP2_SCRN_NBG0, VDP2_SCRN_REDUCTION_QUARTER);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp_sync(0);

        _transfer_cpd();
        _transfer_pal();
        _transfer_pnd(&format);

        vdp2_tvmd_display_set();

        vdp_sync(0);
}

static void
_transfer_cpd(void)
{
        uint32_t tile;
        for (tile = 0; tile < 16; tile++) {
                uint8_t byte;
                byte = (tile << 4) | tile;

                (void)memset((void *)((uint32_t)NBG1_CPD | (tile << 5)), byte, 32);
        }
}

static void
_transfer_pal(void)
{
        (void)memcpy((void *)NBG1_PAL, _palette, sizeof(_palette));
}

static void
_transfer_pnd(const struct vdp2_scrn_cell_format *format)
{
        /* The scroll screen is set up to have 16 64x64 cell pages (4 planes
         * total), each 4 KiB, resulting in 64 KiB.
         *
         * 1. Set up a DMA indirect table with 2 entries
         * 2. Split a single page into two 64x32 sub-pages
         * 3. While one sub-page is being filled, transfer the other sub-page
         *    via the DMA queue
         * 4. Use the two entries to transfer one mirrored sub-page one after
         *    the other
         * 5. Swap sub-pages and repeat */

        uint32_t page_width;
        page_width = VDP2_SCRN_CALCULATE_PAGE_WIDTH(format);
        uint32_t page_height;
        page_height = VDP2_SCRN_CALCULATE_PAGE_HEIGHT(format);
        uint32_t page_size;
        page_size = VDP2_SCRN_CALCULATE_PAGE_SIZE(format);

        struct scu_dma_reg_buffer scu_dma_reg_buffer;

        /* XXX: WA until memalign() is implemented { */
        void *p;
        p = malloc(32 + (2 * sizeof(struct scu_dma_xfer)));
        assert(p != NULL);

        uint32_t aligned_offset;
        aligned_offset = (((uint32_t)p + 0x0000001F) & ~0x0000001F) - (uint32_t)p;

        struct scu_dma_xfer *xfer_table;
        xfer_table = (struct scu_dma_xfer *)((uint32_t)p + aligned_offset);
        /* } */

        struct scu_dma_level_cfg scu_dma_level_cfg = {
                .dlc_mode = SCU_DMA_MODE_INDIRECT,
                .dlc_stride = SCU_DMA_STRIDE_2_BYTES,
                .dlc_update = SCU_DMA_UPDATE_NONE,
                .dlc_xfer.indirect = xfer_table
        };

        scu_dma_config_buffer(&scu_dma_reg_buffer, &scu_dma_level_cfg);

        uint16_t *map[2];
        map[0] = malloc(page_size / 2);
        assert(map[0] != NULL);
        map[1] = malloc(page_size / 2);
        assert(map[1] != NULL);

        uint32_t offset;
        offset = 0;

        uint32_t pnd;
        pnd = NBG1_MAP_PLANE_A;

        uint32_t tile;
        for (tile = 0; tile < 16; tile++) {
                uint16_t *map_p;
                map_p = map[offset];

                _fill_map_pnd(map_p, page_width, page_height / 2, tile);

                dma_queue_flush_wait();

                xfer_table[0].len = page_size / 2;
                xfer_table[0].dst = (uint32_t)pnd;
                xfer_table[0].src = CPU_CACHE_THROUGH | (uint32_t)map_p;

                xfer_table[1].len = page_size / 2;
                xfer_table[1].dst = (uint32_t)pnd | (page_size / 2);
                xfer_table[1].src = SCU_DMA_INDIRECT_TBL_END | CPU_CACHE_THROUGH | (uint32_t)map_p;

                int8_t ret;
                ret = dma_queue_enqueue(&scu_dma_reg_buffer, DMA_QUEUE_TAG_IMMEDIATE, NULL, NULL);
                assert(ret == 0);

                dma_queue_flush(DMA_QUEUE_TAG_IMMEDIATE);

                offset ^= 1;
                pnd += page_size;
        }

        dma_queue_flush_wait();

        free(p);
        free(map[0]);
        free(map[1]);
}

static void
_fill_map_pnd(uint16_t *map, uint16_t page_width, uint16_t page_height, uint16_t tile)
{
        uint16_t x;
        for (x = 0; x < page_width; x++) {
                uint16_t y;
                for (y = 0; y < page_height; y++) {
                        uint32_t cpd;
                        cpd = (uint32_t)NBG1_CPD | (tile << 5);

                        uint16_t pnd;
                        pnd = VDP2_SCRN_PND_CONFIG_1(cpd, NBG1_PAL);

                        map[x + (y * page_width)] = pnd;
                }
        }
}
