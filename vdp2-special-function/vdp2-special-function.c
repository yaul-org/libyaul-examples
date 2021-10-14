/*
 * Copyright (c) 2012-2018 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <sys/queue.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

#define ORDER_SYSTEM_CLIP_COORDS_INDEX  0
#define ORDER_LOCAL_COORDS_INDEX        1
#define ORDER_POLYGON_INDEX             2
#define ORDER_DRAW_END_INDEX            3
#define ORDER_COUNT                     4

#define NBG0_CPD                VDP2_VRAM_ADDR(0, 0x00000)
#define NBG0_PAL                VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define NBG0_MAP_PLANE_A        VDP2_VRAM_ADDR(3, 0x00000)
#define NBG0_MAP_PLANE_B        VDP2_VRAM_ADDR(3, 0x00000)
#define NBG0_MAP_PLANE_C        VDP2_VRAM_ADDR(3, 0x00000)
#define NBG0_MAP_PLANE_D        VDP2_VRAM_ADDR(3, 0x00000)

extern uint8_t root_romdisk[];

static void _cmdt_list_init(vdp1_cmdt_list_t *);

static void _dma_upload(void *, void *, size_t);

void
main(void)
{
        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        void *fh[3];
        void *p;
        size_t len;

        fh[0] = romdisk_open(romdisk, "CHR.DAT");
        assert(fh[0] != NULL);
        p = romdisk_direct(fh[0]);
        len = romdisk_total(fh[0]);
        _dma_upload((void *)NBG0_CPD, p, len);

        fh[1] = romdisk_open(romdisk, "PND.DAT");
        assert(fh[1] != NULL);
        p = romdisk_direct(fh[1]);
        len = romdisk_total(fh[1]);
        _dma_upload((void *)NBG0_MAP_PLANE_A, p, len);

        fh[2] = romdisk_open(romdisk, "PAL.DAT");
        assert(fh[2] != NULL);
        p = romdisk_direct(fh[2]);
        len = romdisk_total(fh[2]);
        _dma_upload((void *)NBG0_PAL, p, len);

        vdp2_tvmd_display_set();

        vdp1_cmdt_list_t *cmdt_list;
        cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);

        _cmdt_list_init(cmdt_list);

        vdp1_sync_cmdt_list_put(cmdt_list, 0, NULL, NULL);
        vdp2_sync();
        vdp2_sync_wait();

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);
        romdisk_close(fh[2]);

        bool show_polygon;
        show_polygon = false;

        uint32_t count_frames;
        count_frames = 0;

        while (true) {
                vdp1_cmdt_t *cmdt_polygon;
                cmdt_polygon = &cmdt_list->cmdts[ORDER_POLYGON_INDEX];

                if (show_polygon) {
                        vdp1_cmdt_jump_clear(cmdt_polygon);
                } else {
                        vdp1_cmdt_jump_skip_next(cmdt_polygon);
                }

                vdp1_sync_cmdt_list_put(cmdt_list, 0, NULL, NULL);

                vdp2_sync();
                vdp2_sync_wait();

                count_frames++;

                if (count_frames >= 15) {
                        count_frames = 0;
                        show_polygon ^= true;
                }
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_clear();

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 0, 7));

        const vdp2_scrn_cell_format_t format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .cc_count = VDP2_SCRN_CCC_PALETTE_2048,
                .character_size = 1 * 1,
                .pnd_size = 2,
                .auxiliary_mode = 1,
                .sf_mode = 2,
                .sf_code = VDP2_SCRN_SF_CODE_A,
                .plane_size = 1 * 1,
                .cp_table = NBG0_CPD,
                .color_palette = NBG0_PAL,
                .map_bases = {
                        .planes = {
                                NBG0_MAP_PLANE_A,
                                NBG0_MAP_PLANE_B,
                                NBG0_MAP_PLANE_C,
                                NBG0_MAP_PLANE_D
                        }
                }
        };

        vdp2_scrn_cell_format_set(&format);

        /* The special priority function only toggles the LSB, so the priority
         * must be even */
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 2);

        /* The lower two bits of the CPD are used to determine which pixel in
         * the character pattern data will be above or below the VDP1 layer.
         *
         * Considering that we need an extra two bits and still make use of
         * 256-colors (8-BPP), paletted 2048 (16-BPP) is the perfect choice for
         * this.
         *
         * We can still designate 8 of the 10 bits within the CPD.
         *
         * The downside is that the 256-color palette needs to be duplicated 4
         * times, resulting in using 1,024 colors in CRAM (exactly half in CRAM
         * mode 1). The reason being is that the special function makes use of
         * the lower 4-bits of the CPD.
         *
         * The codes (0x02, 0x06, 0x0A, and 0x0E) are used. The "odd" codes can
         * also be used: (0x03, 0x07, 0x0B, and 0x0F).
         */

        vdp2_scrn_sf_codes_set(VDP2_SCRN_SF_CODE_A, VDP2_SCRN_SF_CODE_0x02_0x03 |
                                               VDP2_SCRN_SF_CODE_0x06_0x07 |
                                               VDP2_SCRN_SF_CODE_0x0A_0x0B |
                                               VDP2_SCRN_SF_CODE_0x0E_0x0F);

        vdp2_scrn_display_set(VDP2_SCRN_NBG0, /* transparent = */ false);

        /* Each cell is 128 bytes (8x8 cell, at 2-bytes per pixel). To fill a
         * 40x30 cell background, 0x25800 bytes is needed. A single VRAM bank
         * (set at 4-Mbit) is 0x20000 bytes. Without going across bank
         * boundaries, we need to not part a bank into two */
        const vdp2_vram_ctl_t vram_ctl = {
                .vram_mode = VDP2_VRAM_CTL_MODE_NO_PART_BANK_A | VDP2_VRAM_CTL_MODE_PART_BANK_B
        };

        vdp2_vram_control_set(&vram_ctl);

        vdp2_cram_mode_set(1);

        vdp2_vram_cycp_bank_t vram_cycp_bank[2];

        vram_cycp_bank[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        vram_cycp_bank[1].t0 = VDP2_VRAM_CYCP_PNDR_NBG0;
        vram_cycp_bank[1].t1 = VDP2_VRAM_CYCP_PNDR_NBG0;
        vram_cycp_bank[1].t2 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t3 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS;

        /* When bank A (or B) is not parted, cycle patterns for bank A1 does not
         * need to be set */
        vdp2_vram_cycp_bank_set(0, &vram_cycp_bank[0]);
        vdp2_vram_cycp_bank_clear(1);
        vdp2_vram_cycp_bank_clear(2);
        vdp2_vram_cycp_bank_set(3, &vram_cycp_bank[1]);

        const vdp1_env_t vdp1_env = {
                .erase_color = COLOR_RGB1555_INITIALIZER(0, 0, 0, 0),
                .erase_points = {
                        INT16_VEC2_INITIALIZER(0, 0),
                        INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1)
                },
                .bpp = VDP1_ENV_BPP_16,
                .rotation = VDP1_ENV_ROTATION_0,
                .color_mode = VDP1_ENV_COLOR_MODE_RGB_PALETTE,
                .sprite_type = 0x5
        };

        vdp1_env_set(&vdp1_env);

        /* Sprite type 5 has 3 bits dedicated to priority, allowing the
         * selection of 7 of the sprite registers */
        vdp2_sprite_priority_set(0, 2);
        vdp2_sprite_priority_set(1, 2);
        vdp2_sprite_priority_set(2, 2);
        vdp2_sprite_priority_set(3, 2);
        vdp2_sprite_priority_set(4, 2);
        vdp2_sprite_priority_set(5, 2);
        vdp2_sprite_priority_set(6, 2);
        vdp2_sprite_priority_set(7, 2);

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_sync();
        vdp2_sync_wait();
}

static void
_cmdt_list_init(vdp1_cmdt_list_t *cmdt_list)
{
        static const int16_vec2_t system_clip_coord =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                      SCREEN_HEIGHT - 1);

        static const int16_vec2_t local_coord_ul =
            INT16_VEC2_INITIALIZER(0,
                                      0);

        static const vdp1_cmdt_draw_mode_t polygon_draw_mode = {
                .raw = 0x0000,
                .bits.pre_clipping_disable = true
        };

        static const int16_vec2_t polygon_points[] = {
                INT16_VEC2_INITIALIZER(0, SCREEN_HEIGHT - 1),
                INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1),
                INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,                 0),
                INT16_VEC2_INITIALIZER(               0,                 0)
        };

        vdp1_cmdt_t *cmdts;
        cmdts = &cmdt_list->cmdts[0];

        (void)memset(&cmdts[0], 0x00, sizeof(vdp1_cmdt_t) * ORDER_COUNT);

        cmdt_list->count = ORDER_COUNT;

        vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX],
            CMDT_VTX_SYSTEM_CLIP,
            &system_clip_coord);

        vdp1_cmdt_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_LOCAL_COORDS_INDEX],
            CMDT_VTX_LOCAL_COORD, &local_coord_ul);

        vdp1_cmdt_polygon_set(&cmdts[ORDER_POLYGON_INDEX]);
        vdp1_cmdt_param_draw_mode_set(&cmdts[ORDER_POLYGON_INDEX], polygon_draw_mode);
        vdp1_cmdt_param_color_set(&cmdts[ORDER_POLYGON_INDEX], COLOR_RGB1555(1, 15, 7, 7));
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_POLYGON_INDEX], CMDT_VTX_POLYGON_A, &polygon_points[0]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_POLYGON_INDEX], CMDT_VTX_POLYGON_B, &polygon_points[1]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_POLYGON_INDEX], CMDT_VTX_POLYGON_C, &polygon_points[2]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_POLYGON_INDEX], CMDT_VTX_POLYGON_D, &polygon_points[3]);

        vdp1_cmdt_end_set(&cmdts[ORDER_DRAW_END_INDEX]);
}

static void
_dma_upload(void *dst, void *src, size_t len)
{
        scu_dma_level_cfg_t scu_dma_level_cfg;
        scu_dma_handle_t handle;

        scu_dma_level_cfg.mode = SCU_DMA_MODE_DIRECT;
        scu_dma_level_cfg.stride = SCU_DMA_STRIDE_2_BYTES;
        scu_dma_level_cfg.update = SCU_DMA_UPDATE_NONE;
        scu_dma_level_cfg.xfer.direct.len = len;
        scu_dma_level_cfg.xfer.direct.dst = (uint32_t)dst;
        scu_dma_level_cfg.xfer.direct.src = CPU_CACHE_THROUGH | (uint32_t)src;

        scu_dma_config_buffer(&handle, &scu_dma_level_cfg);

        int8_t ret __unused;
        ret = dma_queue_enqueue(&handle, DMA_QUEUE_TAG_VBLANK_IN, NULL, NULL);
        assert(ret == 0);
}
