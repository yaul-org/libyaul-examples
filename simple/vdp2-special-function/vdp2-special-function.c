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

#define NBG0_CPD                VRAM_ADDR_4MBIT(0, 0x00000)
#define NBG0_PAL                CRAM_MODE_1_OFFSET(0, 0, 0)
#define NBG0_MAP_PLANE_A        VRAM_ADDR_4MBIT(3, 0x00000)
#define NBG0_MAP_PLANE_B        VRAM_ADDR_4MBIT(3, 0x00000)
#define NBG0_MAP_PLANE_C        VRAM_ADDR_4MBIT(3, 0x00000)
#define NBG0_MAP_PLANE_D        VRAM_ADDR_4MBIT(3, 0x00000)

extern uint8_t root_romdisk[];

static void _hardware_init(void);

static void _create_drawing_env(struct vdp1_cmdt_list *, bool);

static void _dma_upload(void *, void *, size_t);

void
main(void)
{
        _hardware_init();

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

        struct vdp1_cmdt_list *cmdt_list;
        cmdt_list = vdp1_cmdt_list_alloc(6);

        _create_drawing_env(cmdt_list, false);

        struct vdp1_cmdt_polygon clear_polygon;

        clear_polygon.cp_mode.raw = 0x0000;
        clear_polygon.cp_color.raw = 0x00000;
        clear_polygon.cp_vertex.a.x = 0;
        clear_polygon.cp_vertex.a.y = SCREEN_HEIGHT - 1;

        clear_polygon.cp_vertex.b.x = SCREEN_WIDTH - 1;
        clear_polygon.cp_vertex.b.y = SCREEN_HEIGHT - 1;

        clear_polygon.cp_vertex.c.x = SCREEN_WIDTH - 1;
        clear_polygon.cp_vertex.c.y = 0;

        clear_polygon.cp_vertex.d.x = 0;
        clear_polygon.cp_vertex.d.y = 0;

        vdp1_cmdt_polygon_draw(cmdt_list, &clear_polygon);
        vdp1_cmdt_end(cmdt_list);

        struct vdp1_cmdt_list *cmdt_list_polygon;
        cmdt_list_polygon = vdp1_cmdt_list_alloc(2);

        struct vdp1_cmdt_polygon polygon;

        polygon.cp_mode.raw = 0x0000;
        polygon.cp_color = COLOR_RGB555(31, 0, 0);
        polygon.cp_vertex.a.x = 0;
        polygon.cp_vertex.a.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.b.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.b.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.c.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.c.y = 0;

        polygon.cp_vertex.d.x = 0;
        polygon.cp_vertex.d.y = 0;

        vdp1_cmdt_polygon_draw(cmdt_list_polygon, &polygon);
        vdp1_cmdt_end(cmdt_list_polygon);

        vdp1_sync_draw(cmdt_list);
        vdp2_sync_commit();
        vdp_sync(0);

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);
        romdisk_close(fh[2]);

        bool show_polygon;
        show_polygon = false;

        uint32_t count_frames;
        count_frames = 0;

        while (true) {
                vdp1_sync_draw(cmdt_list);

                if (show_polygon) {
                        vdp1_sync_draw(cmdt_list_polygon);
                }

                vdp2_sync_commit();
                vdp_sync(0);

                count_frames++;

                if (count_frames >= 60) {
                        count_frames = 0;
                        show_polygon ^= true;
                }
        }
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_clear();

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(0, 0, 7));

        const struct scrn_cell_format format = {
                .scf_scroll_screen = SCRN_NBG0,
                .scf_cc_count = SCRN_CCC_PALETTE_2048,
                .scf_character_size = 1 * 1,
                .scf_pnd_size = 2,
                .scf_auxiliary_mode = 1,
                .scf_sf_mode = 2,
                .scf_sf_code = SCRN_SF_CODE_A,
                .scf_plane_size = 1 * 1,
                .scf_cp_table = NBG0_CPD,
                .scf_color_palette = NBG0_PAL,
                .scf_map = {
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
        vdp2_scrn_priority_set(SCRN_NBG0, 2);

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

        vdp2_scrn_sf_codes_set(SCRN_SF_CODE_A, SCRN_SF_CODE_0x02_0x03 |
                                               SCRN_SF_CODE_0x06_0x07 |
                                               SCRN_SF_CODE_0x0A_0x0B |
                                               SCRN_SF_CODE_0x0E_0x0F);

        vdp2_scrn_display_set(SCRN_NBG0, /* transparent = */ false);

        /* Each cell is 128 bytes (8x8 cell, at 2-bytes per pixel). To fill a
         * 40x30 cell background, 0x25800 bytes is needed. A single VRAM bank
         * (set at 4-Mbit) is 0x20000 bytes. Without going across bank
         * boundaries, we need to not part a bank into two */
        const struct vram_ctl vram_ctl = {
                .cram_mode = VRAM_CTL_CRAM_MODE_1,
                .vram_size = VRAM_CTL_SIZE_4MBIT,
                .vram_mode = VRAM_CTL_MODE_NO_PART_BANK_A | VRAM_CTL_MODE_PART_BANK_B
        };

        vdp2_vram_control_set(&vram_ctl);

        struct vram_cycp_bank vram_cycp_bank[2];

        vram_cycp_bank[0].t0 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank[0].t1 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank[0].t2 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank[0].t3 = VRAM_CYCP_CHPNDR_NBG0;
        vram_cycp_bank[0].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[0].t7 = VRAM_CYCP_NO_ACCESS;

        vram_cycp_bank[1].t0 = VRAM_CYCP_PNDR_NBG0;
        vram_cycp_bank[1].t1 = VRAM_CYCP_PNDR_NBG0;
        vram_cycp_bank[1].t2 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t3 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t4 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t5 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t6 = VRAM_CYCP_NO_ACCESS;
        vram_cycp_bank[1].t7 = VRAM_CYCP_NO_ACCESS;

        /* When bank A (or B) is not parted, cycle patterns for bank A1 does not
         * need to be set */
        vdp2_vram_cycp_bank_set(0, &vram_cycp_bank[0]);
        vdp2_vram_cycp_bank_clear(1);
        vdp2_vram_cycp_bank_clear(2);
        vdp2_vram_cycp_bank_set(3, &vram_cycp_bank[1]);

        /* Sprite type 5 has 3 bits dedicated to priority, allowing the
         * selection of 7 of the sprite registers */
        vdp2_sprite_type_set(0x5);
        vdp2_sprite_priority_set(0, 2);
        vdp2_sprite_priority_set(1, 2);
        vdp2_sprite_priority_set(2, 2);
        vdp2_sprite_priority_set(3, 2);
        vdp2_sprite_priority_set(4, 2);
        vdp2_sprite_priority_set(5, 2);
        vdp2_sprite_priority_set(6, 2);
        vdp2_sprite_priority_set(7, 2);

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_240);

        vdp2_sync_commit();
        vdp_sync(0);
}

static void
_create_drawing_env(struct vdp1_cmdt_list *cmdt_list, bool end)
{
        struct vdp1_cmdt_local_coord local_coord = {
                .lc_coord = {
                        .x = 0,
                        .y = 0
                }
        };

        struct vdp1_cmdt_system_clip_coord system_clip = {
                .scc_coord = {
                        .x = SCREEN_WIDTH - 1,
                        .y = SCREEN_HEIGHT - 1
                }
        };

        struct vdp1_cmdt_user_clip_coord user_clip = {
                .ucc_coords = {
                        {
                                .x = 0,
                                .y = 0
                        },
                        {
                                .x = SCREEN_WIDTH - 1,
                                .y = SCREEN_HEIGHT - 1
                        }
                }
        };

        vdp1_cmdt_system_clip_coord_set(cmdt_list, &system_clip);
        vdp1_cmdt_user_clip_coord_set(cmdt_list, &user_clip);
        vdp1_cmdt_local_coord_set(cmdt_list, &local_coord);

        if (end) {
                vdp1_cmdt_end(cmdt_list);
        }
}

static void
_dma_upload(void *dst, void *src, size_t len)
{
        struct dma_level_cfg dma_level_cfg;
        struct dma_reg_buffer reg_buffer;

        dma_level_cfg.dlc_mode = DMA_MODE_DIRECT;
        dma_level_cfg.dlc_stride = DMA_STRIDE_2_BYTES;
        dma_level_cfg.dlc_update = DMA_UPDATE_NONE;
        dma_level_cfg.dlc_xfer.direct.len = len;
        dma_level_cfg.dlc_xfer.direct.dst = (uint32_t)dst;
        dma_level_cfg.dlc_xfer.direct.src = CPU_CACHE_THROUGH | (uint32_t)src;

        scu_dma_config_buffer(&reg_buffer, &dma_level_cfg);

        int8_t ret;
        ret = dma_queue_enqueue(&reg_buffer, DMA_QUEUE_TAG_VBLANK_IN, NULL, NULL);
        assert(ret == 0);
}
