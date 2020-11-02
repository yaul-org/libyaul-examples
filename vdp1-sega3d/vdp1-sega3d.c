/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>
#include <sega3d.h>

#include <assert.h>
#include <stdlib.h>

#define RESOLUTION_WIDTH    (320)
#define RESOLUTION_HEIGHT   (224)

#define SCREEN_WIDTH    (320)
#define SCREEN_HEIGHT   (224)

#define ORDER_SYSTEM_CLIP_COORDS_INDEX  (0)
#define ORDER_LOCAL_COORDS_INDEX        (1)
#define ORDER_SEGA3D_INDEX              (2)
#define ORDER_BASE_COUNT                (3)

extern PDATA PD_PLANE1[];
extern PDATA PD_CUBE1[];

static void _vblank_out_handler(void *);

static void _cmdt_list_init(vdp1_cmdt_list_t *);

static smpc_peripheral_digital_t _digital;

int
main(void)
{
        sega3d_init();

        PDATA *pdata;
        pdata = PD_CUBE1;

        uint16_t polygon_count;
        polygon_count = sega3d_polycount_get(pdata);

        uint16_t cmdt_list_count;
        cmdt_list_count = ORDER_BASE_COUNT + polygon_count;

        vdp1_cmdt_list_t *cmdt_list;
        cmdt_list = vdp1_cmdt_list_alloc(cmdt_list_count);
        assert(cmdt_list != NULL);

        _cmdt_list_init(cmdt_list);

        sega3d_cmdt_prepare(pdata, cmdt_list, ORDER_SEGA3D_INDEX);

        /* Be sure to terminate list */
        vdp1_cmdt_end_set(&cmdt_list->cmdts[cmdt_list_count - 1]);

        /* Set the number of command tables to draw from the list */
        cmdt_list->count = cmdt_list_count;

        MATRIX matrix;

        matrix[0][0] = toFIXED(0.5000000); matrix[0][1] = toFIXED(-0.5000000); matrix[0][2] = toFIXED( 0.7071068);
        matrix[1][0] = toFIXED(0.8535534); matrix[1][1] = toFIXED( 0.1464466); matrix[1][2] = toFIXED(-0.5000000);
        matrix[2][0] = toFIXED(0.1464466); matrix[2][1] = toFIXED( 0.8535534); matrix[2][2] = toFIXED( 0.5000000);

        sega3d_matrix_load(&matrix);

        FIXED z;
        z = toFIXED(0.0f);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                dbgio_printf("[H[2J");
                dbgio_printf("z: %li\n", z);

                sega3d_matrix_push(MATRIX_TYPE_PUSH); {
                        sega3d_matrix_translate(toFIXED(0.0f), toFIXED(0.0f), z);
                        sega3d_cmdt_transform(pdata);
                } sega3d_matrix_pop();

                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_UP) != 0) {
                        z += toFIXED(-1.0f);
                } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_DOWN) != 0) {
                        z += toFIXED( 1.0f);
                }

                vdp1_sync_cmdt_list_put(cmdt_list, NULL, NULL);

                dbgio_flush();
                vdp_sync();
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler);

        vdp1_env_default_set();
        vdp2_sprite_priority_set(0, 6);

        cpu_intc_mask_set(0);

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        vdp2_tvmd_display_set();
}

static void
_cmdt_list_init(vdp1_cmdt_list_t *cmdt_list)
{
        vdp1_cmdt_t *cmdt;
        cmdt = &cmdt_list->cmdts[0];

        static const int16_vector2_t local_coords =
            INT16_VECTOR2_INITIALIZER(RESOLUTION_WIDTH / 2,
                RESOLUTION_HEIGHT / 2);

        static const int16_vector2_t system_clip_coords =
            INT16_VECTOR2_INITIALIZER(RESOLUTION_WIDTH,
                RESOLUTION_HEIGHT);

        vdp1_cmdt_system_clip_coord_set(&cmdt[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdt[ORDER_SYSTEM_CLIP_COORDS_INDEX],
            CMDT_VTX_SYSTEM_CLIP, &system_clip_coords);

        vdp1_cmdt_local_coord_set(&cmdt[ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdt[ORDER_LOCAL_COORDS_INDEX],
            CMDT_VTX_LOCAL_COORD, &local_coords);
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}
