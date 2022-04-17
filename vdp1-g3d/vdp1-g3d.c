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

#include "s3d.h"

#define RESOLUTION_WIDTH  (352)
#define RESOLUTION_HEIGHT (240)

#define SCREEN_WIDTH  (352)
#define SCREEN_HEIGHT (240)

#define VDP1_VRAM_CMDT_COUNT    (8192)
#define VDP1_VRAM_TEXTURE_SIZE  (0x3BFE0)
#define VDP1_VRAM_GOURAUD_COUNT (1024)
#define VDP1_VRAM_CLUT_COUNT    (256)

#define ORDER_SYSTEM_CLIP_COORDS_INDEX (0)
#define ORDER_USER_CLIP_INDEX          (1)
#define ORDER_LOCAL_COORDS_INDEX       (2)
#define ORDER_ERASE_INDEX              (3)
#define ORDER_SEGA3D_INDEX             (4)
#define ORDER_BASE_COUNT               (5)

extern uint8_t asset_s3d[];

static void _vblank_in_handler(void *);
static void _vblank_out_handler(void *);
static void _sprite_end_handler(void);

static void _vdp1_init(void);

static void _perf_print(const char *name, const perf_counter_t *perf_counter);

static smpc_peripheral_digital_t _digital;

static vdp1_cmdt_orderlist_t _cmdt_orderlist[VDP1_VRAM_CMDT_COUNT] __aligned(0x20000);
static vdp1_cmdt_t *_cmdts;
static TEXTURE _textures[1024];
static PALETTE _palettes[512];

static perf_counter_t _perf_vdp1;
static vdp1_vram_partitions_t _vram_partitions;

static void __used
_sync(void)
{
        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();
        abort();
}

int
main(void)
{
        _vdp1_init();

        sega3d_init();
        sega3d_display_level_set(3);
        sega3d_tlist_set(_textures, 1024);
        sega3d_plist_set(_palettes, 512);

        sega3d_s3d_t * const s3d = (sega3d_s3d_t *)asset_s3d;

        sega3d_s3d_object_t * const s3d_objects =
            sega3d_s3d_objects_get(s3d);

        sega3d_s3d_memory_usage_t memory_usage = {
                .texture       = _vram_partitions.texture_base,
                .clut          = _vram_partitions.clut_base,
                .gouraud_table = _vram_partitions.gouraud_base,
                .cram          = (vdp2_cram_t *)VDP2_CRAM(0x0200)
        };

        sega3d_s3d_patch(s3d, &memory_usage);

        /* Copy over gouraud tables */
        if (s3d_objects[0].gouraud_table_count > 0) {
                vdp1_gouraud_table_t * const gouraud_table_base =
                    sega3d_s3d_object_gtb_base_get(&s3d_objects[0]);

                scu_dma_transfer(2,
                    gouraud_table_base,
                    s3d_objects[0].gouraud_tables,
                    s3d_objects[0].gouraud_table_count * sizeof(vdp1_gouraud_table_t));
                scu_dma_transfer_wait(2);
        }

        dbgio_printf("%s\n", s3d->sig);
        dbgio_printf("%04X\n", s3d->version);
        dbgio_printf("obj_count %lu\n", s3d->object_count);
        dbgio_printf("tex_count %lu\n", s3d->textures_count);
        dbgio_printf("pic_count %lu\n", s3d_objects[0].picture_count);
        dbgio_printf("textures ptr 0x%p\n", (uintptr_t)s3d->textures);
        dbgio_printf("texture_datas ptr 0x%p\n", (uintptr_t)s3d->texture_datas);
        dbgio_printf("palette_datas ptr 0x%p\n", (uintptr_t)s3d->palette_datas);

        dbgio_printf("Texture VRAM: 0x%p\n", _vram_partitions.texture_base);

        for (uint32_t i = 0; i < s3d_objects[0].picture_count; i++) {
                PICTURE * const picture = s3d_objects[0].pictures;
                sega3d_s3d_texture_t * const texture = picture->pcsrc;

                TEXTURE * const texturesgl = &s3d->textures[picture->texno];

                dbgio_printf("texture [%04i]: 0x%p/0x%04X -> (%ix%i) 0x%08X\n",
                    picture->texno,
                    (uintptr_t)sega3d_s3d_texture_base_get(texture),
                    sega3d_s3d_texture_size_get(texture),
                    texturesgl->Hsize,
                    texturesgl->Vsize,
                    VDP1_VRAM(texturesgl->CGadr << 3));
        }

        sega3d_s3d_texture_t * texture;
        texture = s3d->texture_datas;

        for (uint32_t tex_no = 0; ; tex_no++) {
                TEXTURE * const texturesgl = &s3d->textures[tex_no];

                dbgio_printf("Transfer TEXNO%i -> 0x%08X (0x%04X)\n",
                    tex_no,
                    (void *)VDP1_VRAM(texturesgl->CGadr << 3),
                    sega3d_s3d_texture_size_get(texture));

                scu_dma_transfer(0,
                    (void *)VDP1_VRAM(texturesgl->CGadr << 3),
                    sega3d_s3d_texture_base_get(texture),
                    sega3d_s3d_texture_size_get(texture));
                scu_dma_transfer_wait(0);

                if (texture->eol) {
                        break;
                }

                texture = texture->next;
        }

        (void)memcpy(_textures, s3d->textures, min(s3d->textures_count, 1024UL) * sizeof(TEXTURE));
        (void)memcpy(_palettes, s3d->palettes, min(s3d->palettes_count, 512UL) * sizeof(PALETTE));

        sega3d_s3d_palette_t *palette;
        palette = s3d->palette_datas;

        for (uint32_t pal_no = 0; ; pal_no++) {
                PALETTE * const palettesgl = &s3d->palettes[pal_no];

                void *addr;

                if (palettesgl->Color) {
                        addr = (void *)VDP2_CRAM(palettesgl->Color << 5);
                } else {
                        addr = (void *)VDP1_VRAM(palettesgl->Color << 5);
                }

                dbgio_printf("Transfer PALNO%i -> 0x%08X (0x%04X) (0x%04X)\n",
                    pal_no,
                    addr,
                    sega3d_s3d_palette_size_get(palette),
                    *(uint16_t *)palettesgl);

                scu_dma_transfer(0,
                    addr,
                    sega3d_s3d_palette_base_get(palette),
                    sega3d_s3d_palette_size_get(palette));
                scu_dma_transfer_wait(0);

                if (palette->eol) {
                        break;
                }

                palette = palette->next;
        }

        /* _sync(); */

        /* sega3d_cull_aabb_t aabb; */

        XPDATA xpdatas[1];

        memcpy(xpdatas, &s3d_objects[0].xpdata, sizeof(XPDATA));
        /* xpdatas[0] = s3d_objects[0].xpdata; */

        sega3d_object_t object;
        object.flags = SEGA3D_OBJECT_FLAGS_CULL_SCREEN | SEGA3D_OBJECT_FLAGS_FOG_EXCLUDE;
        object.xpdatas = xpdatas;
        object.xpdata_count = s3d->object_count;
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
                        sega3d_frustum_camera_set(camera_pos, rot_x, rot_y, rot_z);

                        sega3d_matrix_push(SEGA3D_MATRIX_TYPE_PUSH); {
                                sega3d_matrix_rot_y(rot[Y]);
                                sega3d_matrix_rot_x(rot[X]);
                                sega3d_matrix_rot_z(rot[Z]);

                                for (uint32_t i = 0; i < object.xpdata_count; i++) {
                                        sega3d_object_transform(&object, i);
                                }
                        } sega3d_matrix_pop();
                } sega3d_matrix_pop();

                sega3d_finish(&results);

                perf_counter_start(&_perf_vdp1);
                vdp1_sync_render();
                perf_counter_end(&_perf_vdp1);

                vdp1_sync();
                vdp2_sync();
                vdp1_sync_wait();

                dbgio_puts("[5;1H");
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


                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_X) != 0) {
                        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_LEFT) != 0) {
                                rot[X] -= DEGtoANG(1.0f);
                        } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_RIGHT) != 0) {
                                rot[X] += DEGtoANG(1.0f);
                        }
                }

                else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_Y) != 0) {
                        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_LEFT) != 0) {
                                rot[Y] -= DEGtoANG(1.0f);
                        } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_RIGHT) != 0) {
                                rot[Y] += DEGtoANG(1.0f);
                        }
                }

                else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_Z) != 0) {
                        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_LEFT) != 0) {
                                rot[Z] -= DEGtoANG(1.0f);
                        } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_RIGHT) != 0) {
                                rot[Z] += DEGtoANG(1.0f);
                        }
                } else {

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

static void
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
