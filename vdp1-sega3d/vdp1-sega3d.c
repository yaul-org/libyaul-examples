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

#define RESOLUTION_WIDTH    (320)
#define RESOLUTION_HEIGHT   (224)

#define SCREEN_WIDTH    (320)
#define SCREEN_HEIGHT   (224)

#define VDP1_VRAM_CMDT_COUNT    (8192)
#define VDP1_VRAM_TEXTURE_SIZE  (0x3BFE0)
#define VDP1_VRAM_GOURAUD_COUNT (1024)
#define VDP1_VRAM_CLUT_COUNT    (256)

#define ORDER_SYSTEM_CLIP_COORDS_INDEX  (0)
#define ORDER_USER_CLIP_INDEX           (1)
#define ORDER_LOCAL_COORDS_INDEX        (2)
#define ORDER_SEGA3D_INDEX              (3)
#define ORDER_BASE_COUNT                (3)

extern PDATA PD_PLANE1[];
extern PDATA PD_CUBE1[];
extern PDATA PD_SONIC[];
extern PDATA PD_QUAKE[];
extern PDATA PD_QUAKE_SINGLE_0[];
extern PDATA PD_QUAKE_SINGLE_1[];
extern PDATA PD_QUAKE_SINGLE_2[];
extern PDATA PD_QUAKE_SINGLE_3[];
extern PDATA PD_QUAKE_SINGLE_4[];
extern PDATA PD_TORUS[];

extern Uint16 GR_SMS[];
extern PDATA PD_SMS3[];

extern TEXTURE TEX_SAMPLE[];
extern PICTURE PIC_SAMPLE[];

static void _vblank_in_handler(void *);
static void _vblank_out_handler(void *);
static void _frt_ovi_handler(void);

static void _timing_start(void) __used;
static fix16_t _timing_get(void) __used;
static void _timing_print(const fix16_t time);
static void _assets_copy(const sega3d_object_t *object);

static smpc_peripheral_digital_t _digital;

static volatile uint16_t _vblanks = 0;
static volatile uint16_t _frt_ovf_count = 0;

static vdp1_cmdt_orderlist_t _cmdt_orderlist[VDP1_VRAM_CMDT_COUNT] __aligned(0x20000);
static vdp1_cmdt_t *_cmdts;

static vdp1_vram_partitions_t _vram_partitions;

static void
_divu_ovfi_handler(void)
{
}

static void
_dma_illegal_handler(void)
{
}

int
main(void)
{
        sega3d_init();

        /* Set up global command table list */
        _cmdts = memalign(sizeof(vdp1_cmdt_t) * VDP1_VRAM_CMDT_COUNT, sizeof(vdp1_cmdt_t));
        assert(_cmdts != NULL);

        /* Set up the first few command tables */
        int16_vec2_t p __unused;
        int16_vec2_zero(&p);
        vdp1_env_preamble_populate(&_cmdts[0], NULL);

        vdp1_cmdt_orderlist_init(_cmdt_orderlist, VDP1_VRAM_CMDT_COUNT);

        _cmdt_orderlist[0].cmdt = &_cmdts[0];
        _cmdt_orderlist[1].cmdt = &_cmdts[1];
        _cmdt_orderlist[2].cmdt = &_cmdts[2];
        _cmdt_orderlist[3].cmdt = &_cmdts[3];

        vdp1_cmdt_orderlist_vram_patch(_cmdt_orderlist,
            VDP1_VRAM(0), VDP1_VRAM_CMDT_COUNT);

        sega3d_object_t object;

        object.pdata = PD_QUAKE_SINGLE_2;
        object.flags = SEGA3D_OBJECT_FLAGS_WIREFRAME | SEGA3D_OBJECT_FLAGS_CULL_SCREEN;
        object.data = NULL;

        /* sega3d_tlist_set(TEX_SAMPLE, 2); */

        _assets_copy(&object);

        ANGLE rot[XYZ];
        rot[X] = 0;
        rot[Y] = 0;
        rot[Z] = DEGtoANG(0.0f);

        FIXED translate[XYZ];
        translate[X] = 0;
        translate[Y] = FIX16(15.0f);
        translate[Z] = FIX16(-400.0f);

        sega3d_results_t results __unused;

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                _vblanks = 0;

                sega3d_start(_cmdt_orderlist, ORDER_SEGA3D_INDEX, &_cmdts[ORDER_SEGA3D_INDEX]);

                sega3d_matrix_push(MATRIX_TYPE_PUSH); {
                        sega3d_matrix_rotate_z(rot[Z]);
                        sega3d_matrix_rotate_x(rot[X]);
                        sega3d_matrix_rotate_y(rot[Y]);

                        sega3d_matrix_translate(translate[X], translate[Y], translate[Z]);

                        fix16_t time[16];
                        fix16_t total_time = 0;
                        fix16_t *time_p = time;

                        _timing_start();
                        object.pdata = PD_QUAKE_SINGLE_0;
                        sega3d_object_transform(&object);
                        *time_p = _timing_get(); total_time += *time_p; time_p++;

                        _timing_start();
                        object.pdata = PD_QUAKE_SINGLE_1;
                        sega3d_object_transform(&object);
                        *time_p = _timing_get(); total_time += *time_p; time_p++;

                        _timing_start();
                        object.pdata = PD_QUAKE_SINGLE_2;
                        sega3d_object_transform(&object);
                        *time_p = _timing_get(); total_time += *time_p; time_p++;

                        _timing_start();
                        object.pdata = PD_QUAKE_SINGLE_3;
                        sega3d_object_transform(&object);
                        *time_p = _timing_get(); total_time += *time_p; time_p++;

                        _timing_start();
                        object.pdata = PD_QUAKE_SINGLE_4;
                        sega3d_object_transform(&object);
                        *time_p = _timing_get(); total_time += *time_p; time_p++;

                        for (uint32_t i = 0; i < (uint32_t)(time_p - time); i++) {
                                _timing_print(time[i]);
                        }
                        _timing_print(total_time);
                } sega3d_matrix_pop();
                
                sega3d_finish(&results);

                int8_t dir;
                dir = -1;

                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_X) != 0) {
                        dir = X;
                }
                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_Y) != 0) {
                        dir = Y;
                }
                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_Z) != 0) {
                        dir = Z;
                }

                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_UP) != 0) {
                        if (dir >= 0) {
                                rot[dir] += DEGtoANG(10.0f);
                        } else {
                                translate[Z] += FIX16(5.0f);
                        }
                } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_DOWN) != 0) {
                        if (dir >= 0) {
                                rot[dir] -= DEGtoANG(10.0f);
                        } else {
                                translate[Z] -= FIX16(5.0f);
                        }
                }

                dbgio_flush();
                vdp_sync();

                dbgio_printf("[H[2J");
                dbgio_printf("_vblanks: %u, count: %lu, (%f,%f,%f)\n", _vblanks, results.count, translate[X], translate[Y], translate[Z]);
        }
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_in_set(_vblank_in_handler);
        vdp_sync_vblank_out_set(_vblank_out_handler);

        vdp1_vram_partitions_set(VDP1_VRAM_CMDT_COUNT,
            VDP1_VRAM_TEXTURE_SIZE,
            VDP1_VRAM_GOURAUD_COUNT,
            VDP1_VRAM_CLUT_COUNT);

        vdp1_vram_partitions_get(&_vram_partitions);

        /* Variable internal */
        vdp1_sync_interval_set(-1);

        vdp1_env_default_set();
        vdp2_sprite_priority_set(0, 6);

        cpu_frt_init(CPU_FRT_CLOCK_DIV_128);
        cpu_frt_ovi_set(_frt_ovi_handler);

        cpu_divu_ovfi_set(_divu_ovfi_handler);
        scu_dma_illegal_set(_dma_illegal_handler);

        cpu_cache_purge();

        cpu_intc_mask_set(0);

        dbgio_dev_default_init(DBGIO_DEV_USB_CART);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        vdp2_tvmd_display_set();
}

void
_vblank_in_handler(void *work __unused)
{
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();

        _vblanks++;

        if (_vblanks > 1024) {
                cpu_intc_mask_set(0);
                smpc_smc_resenab_call();
                smpc_smc_sysres_call();

                while (true) {
                }
        }
}

static void
_frt_ovi_handler(void)
{
        _frt_ovf_count++;
}

static void
_timing_start(void)
{
        cpu_frt_count_set(0);
        _frt_ovf_count = 0;
}

static fix16_t
_timing_get(void)
{
        const uint32_t ticks_rem = cpu_frt_count_get();

        const uint32_t overflow_count =
            ((65536 / CPU_FRT_NTSC_320_128_COUNT_1MS) * _frt_ovf_count);

        const uint32_t total_ticks =
            (fix16_int32_from(overflow_count) + (fix16_int32_from(ticks_rem) / CPU_FRT_NTSC_320_128_COUNT_1MS));

        return total_ticks;
}

static void
_timing_print(const fix16_t time)
{
        dbgio_printf("time: %fms, ticks: %li\n", time, time);
}

static void
_assets_copy(const sega3d_object_t *object __unused)
{
        const uint16_t polygon_count __unused =
            sega3d_object_polycount_get(object);

        /* (void)memcpy((uint16_t *)VDP1_VRAM(0x2BFE0), */
        /*     GR_SMS, */
        /*     sizeof(vdp1_gouraud_table_t) * polygon_count); */

        vdp1_vram_partitions_t vdp1_vram_parts;
        vdp1_vram_partitions_get(&vdp1_vram_parts);
        
        for (uint32_t i = 0; i < 2; i++) {
                const PICTURE *picture;
                picture = &PIC_SAMPLE[i];
                const TEXTURE *texture;
                texture = &TEX_SAMPLE[picture->texno];

                const uint32_t vram_ptr = VDP1_VRAM(texture->CGadr << 3);

                const uint32_t size =
                    (texture->Hsize * texture->Vsize * 4) >> picture->cmode;

                (void)memcpy((void *)vram_ptr, picture->pcsrc, size);
        }
}
