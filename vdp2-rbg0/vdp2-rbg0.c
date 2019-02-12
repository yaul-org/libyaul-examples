#include <yaul.h>

#include <sys/queue.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern uint8_t root_romdisk[];

static const struct scrn_rotation_table _rot_tbl __used = {
        .xst = 0,
        .yst = 0,
        .zst = 0,

        .delta_xst = 0x00000000,
        .delta_yst = 0x00010000,

        .delta_x = 0x00010000,
        .delta_y = 0x00000000,

        .matrix = {
                .param.a = 0x00010000,
                .param.b = 0x00000000,
                .param.c = 0x00000000,
                .param.d = 0x00000000,
                .param.e = 0x00010000,
                .param.f = 0x00000000
        },

        .px = 0,
        .py = 0,
        .pz = 0,

        .cx = 0,
        .cy = 0,
        .cz = 0,

        .mx = 0,
        .my = 0,

        .kx = 0x00010000,
        .ky = 0x00010000,

        .kast = 0,
        .delta_kast = 0,
        .delta_kax = 0,
};

static void _hardware_init(void);

void
main(void)
{
        _hardware_init();

        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        void *fh;

        fh = romdisk_open(romdisk, "/TILESET.CEL");
        assert(fh != NULL);
        (void)memcpy((void *)VRAM_ADDR_4MBIT(0, 0x10000), romdisk_direct(fh), romdisk_total(fh));
        romdisk_close(fh);

        fh = romdisk_open(romdisk, "/TILESET.PAL");
        assert(fh != NULL);
        (void)memcpy((void *)CRAM_ADDR(0x0000), romdisk_direct(fh), romdisk_total(fh));
        romdisk_close(fh);

        fh = romdisk_open(romdisk, "/LEVEL1.MAP");
        assert(fh != NULL);
        (void)memcpy((void *)VRAM_ADDR_4MBIT(1, 0x00000), romdisk_direct(fh), romdisk_total(fh));
        romdisk_close(fh);

        (void)memcpy((void *)VRAM_ADDR_4MBIT(2, 0x00000), &_rot_tbl, sizeof(_rot_tbl));

        vdp_sync(0);

        while (true) {
        }
}

static void
_hardware_init(void)
{
        const struct scrn_cell_format format = {
                .scf_scroll_screen = SCRN_RBG0,
                .scf_cc_count = SCRN_CCC_PALETTE_256,
                .scf_character_size = 2 * 2,
                .scf_pnd_size = 1, /* 1-word */
                .scf_auxiliary_mode = 0,
                .scf_plane_size = 2 * 2,
                .scf_cp_table = (uint32_t)VRAM_ADDR_4MBIT(0, 0x10000),
                .scf_color_palette = (uint32_t)CRAM_MODE_1_OFFSET(0, 0, 0),
                .scf_map = {
                        .plane_a = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_b = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_c = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_d = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),

                        .plane_e = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_f = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_g = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_h = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),

                        .plane_i = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_j = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_k = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_l = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),

                        .plane_m = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_n = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_o = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000),
                        .plane_p = (uint32_t)VRAM_ADDR_4MBIT(1, 0x00000)
                },
                .scf_rotation_tbl = VRAM_ADDR_4MBIT(2, 0x00000),
                .scf_usage_banks.a0 = VRAM_USAGE_TYPE_CPD,
                .scf_usage_banks.a1 = VRAM_USAGE_TYPE_PND,
                .scf_usage_banks.b0 = VRAM_USAGE_TYPE_NONE,
                .scf_usage_banks.b1 = VRAM_USAGE_TYPE_NONE
        };

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(5, 5, 7));

        vdp2_tvmd_display_clear();

        vdp2_scrn_cell_format_set(&format);

        vdp2_scrn_priority_set(SCRN_RBG0, 3);
        vdp2_scrn_display_set(SCRN_RBG0, /* transparent = */ false);

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_vram_cycp_clear();

        vdp2_tvmd_display_set();
}
