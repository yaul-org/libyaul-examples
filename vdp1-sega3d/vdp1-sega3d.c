/*
 * Copyright (c) 2012-2020 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>
#include <sega3d.h>

#include <assert.h>
#include <stdlib.h>

#define RESOLUTION_WIDTH    (352)
#define RESOLUTION_HEIGHT   (240)

#define SCREEN_WIDTH    (352)
#define SCREEN_HEIGHT   (240)

#define VDP1_VRAM_CMDT_COUNT    (8192)
#define VDP1_VRAM_TEXTURE_SIZE  (0x3BFE0)
#define VDP1_VRAM_GOURAUD_COUNT (1024)
#define VDP1_VRAM_CLUT_COUNT    (256)

#define ORDER_SYSTEM_CLIP_COORDS_INDEX  (0)
#define ORDER_USER_CLIP_INDEX           (1)
#define ORDER_LOCAL_COORDS_INDEX        (2)
#define ORDER_ERASE_INDEX               (3)
#define ORDER_SEGA3D_INDEX              (4)
#define ORDER_BASE_COUNT                (5)

static void _vblank_in_handler(void *);
static void _vblank_out_handler(void *);
static void _sprite_end_handler(void);

static void _vdp1_init(void);

static void _perf_print(const char *name, const perf_counter_t *perf_counter);

static perf_counter_t _perf_vdp1;

static smpc_peripheral_digital_t _digital;

static vdp1_cmdt_orderlist_t _cmdt_orderlist[VDP1_VRAM_CMDT_COUNT] __aligned(0x20000);
static vdp1_cmdt_t *_cmdts;

static TEXTURE _textures[64];

static vdp1_vram_partitions_t _vram_partitions;

extern XPDATA XDATA_S3D[];
extern uint32_t XPDATA_S3D_COUNT;

int
main(void)
{
        sega3d_init();
        sega3d_display_level_set(3);

        sega3d_tlist_set(_textures, 64);

        _vdp1_init();

        static sega3d_cull_aabb_t aabb;

        sega3d_object_t object;
        object.flags = SEGA3D_OBJECT_FLAGS_WIREFRAME |
                       SEGA3D_OBJECT_FLAGS_CULL_SCREEN |
                       SEGA3D_OBJECT_FLAGS_CULL_VIEW |
                       SEGA3D_OBJECT_FLAGS_NON_TEXTURED;
        object.xpdatas = &XDATA_S3D[0];
        object.xpdata_count = XPDATA_S3D_COUNT;
        object.cull_shape = NULL;
        object.user_data = NULL;

        ANGLE rot[XYZ] __unused;
        rot[X] = DEGtoANG(0.0f);
        rot[Y] = DEGtoANG(0.0f);
        rot[Z] = DEGtoANG(0.0f);

        sega3d_results_t results;

        POINT camera_pos;
        VECTOR rot_x;
        VECTOR rot_y;
        VECTOR rot_z;

        camera_pos[X] = toFIXED(0.0f);
        camera_pos[Y] = toFIXED(0.0f);
        camera_pos[Z] = toFIXED(-100.0f);

        rot_x[X] = rot_x[Y] = rot_x[Z] = toFIXED(0.0f);
        rot_y[X] = rot_y[Y] = rot_y[Z] = toFIXED(0.0f);
        rot_z[X] = rot_z[Y] = rot_z[Z] = toFIXED(0.0f);

        rot_x[X] = toFIXED(0.0f);
        rot_y[Y] = toFIXED(0.0f);
        rot_z[Z] = toFIXED(0.0f);

        dbgio_puts("[H[2J");

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                sega3d_start(_cmdt_orderlist, 0, &_cmdts[0]);

                sega3d_matrix_push(SEGA3D_MATRIX_TYPE_PUSH); {
                        /* sega3d_matrix_rot_y(rot[Y]); */
                        /* sega3d_matrix_rot_x(rot[X]); */
                        /* sega3d_matrix_rot_z(rot[Z]); */

                        sega3d_frustum_camera_set(camera_pos, rot_x, rot_y, rot_z);

                        for (uint32_t i = 0; i < object.xpdata_count; i++) {
                                sega3d_object_transform(&object, i);
                        }
                } sega3d_matrix_pop();

                sega3d_finish(&results);

                perf_counter_start(&_perf_vdp1);
                vdp1_sync_render();
                perf_counter_end(&_perf_vdp1);

                vdp1_sync();
                vdp2_sync();
                vdp1_sync_wait();

                dbgio_puts("[H");
                _perf_print("            sort", &results.perf_sort);
                _perf_print("             dma", &results.perf_dma);
                _perf_print("    aabb_culling", &results.perf_aabb_culling);
                _perf_print("       transform", &results.perf_transform);
                _perf_print("        clipping", &results.perf_clipping);
                _perf_print(" polygon_process", &results.perf_polygon_process);
                _perf_print("            vdp1", &_perf_vdp1);

                const uint32_t total_ticks =
                    results.perf_sort.ticks +
                    results.perf_dma.ticks +
                    results.perf_aabb_culling.ticks +
                    results.perf_transform.ticks +
                    results.perf_clipping.ticks +
                    results.perf_polygon_process.ticks +
                    _perf_vdp1.ticks;

                dbgio_printf("           Total: %010lu\n"
                             "   Polygon count: %010lu\n",
                    total_ticks,
                    results.polygon_count);
                dbgio_flush();

                if (_digital.held.button.start != 0) {
                        smpc_smc_sysres_call();

                        return 0;
                }

                if ((_digital.held.raw & PERIPHERAL_DIGITAL_A) != 0) {
                        object.flags = object.flags ^ SEGA3D_OBJECT_FLAGS_CULL_SCREEN;
                } else if ((_digital.held.raw & PERIPHERAL_DIGITAL_B) != 0) {
                        object.flags = object.flags ^ SEGA3D_OBJECT_FLAGS_WIREFRAME;
                }

                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_LEFT) != 0) {
                        camera_pos[X] -= FIX16(5.0f);
                } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_RIGHT) != 0) {
                        camera_pos[X] += FIX16(5.0f);
                }

                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_UP) != 0) {
                        camera_pos[Z] += FIX16(5.0f);
                } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_DOWN) != 0) {
                        camera_pos[Z] -= FIX16(5.0f);
                }
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_B,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_in_set(_vblank_in_handler, NULL);
        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp1_vram_partitions_set(VDP1_VRAM_CMDT_COUNT,
                                 VDP1_VRAM_TEXTURE_SIZE,
                                 VDP1_VRAM_GOURAUD_COUNT,
                                 VDP1_VRAM_CLUT_COUNT);

        vdp1_vram_partitions_get(&_vram_partitions);

        vdp1_sync_interval_set(-1);

        const vdp1_env_t vdp1_env = {
                .erase_color     = COLOR_RGB1555(0, 0, 0, 0),
                .erase_points[0] = INT16_VEC2_INITIALIZER(0, 0),
                .erase_points[1] = INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1),
                .bpp             = VDP1_ENV_BPP_16,
                .rotation        = VDP1_ENV_ROTATION_0,
                .color_mode      = VDP1_ENV_COLOR_MODE_RGB_PALETTE,
                .sprite_type     = 0
        };

        vdp1_env_set(&vdp1_env);

        vdp2_sprite_priority_set(0, 6);

        cpu_cache_purge();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        vdp2_tvmd_display_set();

        vdp2_sync();
        vdp2_sync_wait();
}

void
_vdp1_init(void)
{
        /* Set up global command table list */
        _cmdts = memalign(sizeof(vdp1_cmdt_t) * VDP1_VRAM_CMDT_COUNT, sizeof(vdp1_cmdt_t));
        assert(_cmdts != NULL);

        vdp1_cmdt_t * const preamble_cmdt = (vdp1_cmdt_t *)VDP1_CMD_TABLE(0, 0);

        /* Set up the first few command tables */
        vdp1_env_preamble_populate(&preamble_cmdt[ORDER_SYSTEM_CLIP_COORDS_INDEX], NULL);

        const vdp1_cmdt_draw_mode_t erase_draw_mode = {
                .raw                       = 0x0000,
                .bits.pre_clipping_disable = true,
                .bits.end_code_disable     = true,
                .bits.trans_pixel_disable  = true
        };

        vdp1_cmdt_polygon_set(&preamble_cmdt[ORDER_ERASE_INDEX]);
        vdp1_cmdt_param_draw_mode_set(&preamble_cmdt[ORDER_ERASE_INDEX], erase_draw_mode);
        vdp1_cmdt_param_color_set(&preamble_cmdt[ORDER_ERASE_INDEX], COLOR_RGB1555(0, 0, 0, 0));

        /* The clear polygon is cut in half due to the VDP1 not having enough
         * time to clear the bottom half of the framebuffer in time */
        preamble_cmdt[ORDER_ERASE_INDEX].cmd_xa = -SCREEN_WIDTH / 2;
        preamble_cmdt[ORDER_ERASE_INDEX].cmd_ya = -18;
        preamble_cmdt[ORDER_ERASE_INDEX].cmd_xb = (SCREEN_WIDTH / 2) - 1;
        preamble_cmdt[ORDER_ERASE_INDEX].cmd_yb = -18;
        preamble_cmdt[ORDER_ERASE_INDEX].cmd_xc = (SCREEN_WIDTH / 2) - 1;
        preamble_cmdt[ORDER_ERASE_INDEX].cmd_yc = SCREEN_HEIGHT - 1;
        preamble_cmdt[ORDER_ERASE_INDEX].cmd_xd = -SCREEN_WIDTH / 2;
        preamble_cmdt[ORDER_ERASE_INDEX].cmd_yd = (SCREEN_HEIGHT / 2) - 1;

        vdp1_cmdt_end_set(&preamble_cmdt[ORDER_SEGA3D_INDEX]);

        vdp1_cmdt_orderlist_init(_cmdt_orderlist, VDP1_VRAM_CMDT_COUNT);

        vdp1_cmdt_orderlist_vram_patch(_cmdt_orderlist,
            (const vdp1_cmdt_t *)VDP1_CMD_TABLE(ORDER_SEGA3D_INDEX, 0),
            VDP1_VRAM_CMDT_COUNT - ORDER_SEGA3D_INDEX);

        scu_ic_ihr_set(SCU_IC_INTERRUPT_SPRITE_END, _sprite_end_handler);
}

static void
_perf_print(const char *name, const perf_counter_t *perf_counter)
{
        dbgio_printf("%s: %10lu/%10lu\n", name, perf_counter->ticks, perf_counter->max_ticks);

        if ((dbgio_dev_selected_get()) == DBGIO_DEV_USB_CART) {
                dbgio_flush();
        }
}

void
_vblank_in_handler(void *work __unused)
{
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_sprite_end_handler(void)
{
        perf_counter_end(&_perf_vdp1);
}
