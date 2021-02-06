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

extern uint8_t root_romdisk[];

static void _vblank_in_handler(void *);
static void _vblank_out_handler(void *);
static void _frt_ovi_handler(void);

static void _timing_start(void) __used;
static fix16_t _timing_get(void) __used;
static void _timing_print(const fix16_t time) __used;

static smpc_peripheral_digital_t _digital;

static volatile uint16_t _vblanks = 0;
static volatile uint16_t _frt_ovf_count = 0;

static vdp1_cmdt_orderlist_t _cmdt_orderlist[VDP1_VRAM_CMDT_COUNT] __aligned(0x20000);
static vdp1_cmdt_t *_cmdts;

static TEXTURE _textures[64];

static vdp1_vram_partitions_t _vram_partitions;

typedef struct {
        char sig[4];
        uint16_t version;
        uint32_t flags;
        uint16_t xpdata_count;
        uint16_t picture_count;
} __packed __aligned(64) sega3d_s3d_t;

typedef struct {
        void *gouraud_table;
        uint16_t gouraud_table_count;
        void *cg;
        uint32_t cg_size;
} __packed __aligned(16) sega3d_s3d_aux_t;

static_assert(sizeof(sega3d_s3d_t) == 64);

int
main(void)
{
        sega3d_init();
        sega3d_display_level_set(3);

        sega3d_tlist_set(_textures, 64);

        /* Set up global command table list */
        _cmdts = memalign(sizeof(vdp1_cmdt_t) * VDP1_VRAM_CMDT_COUNT, sizeof(vdp1_cmdt_t));
        assert(_cmdts != NULL);
        /* Set up the first few command tables */
        vdp1_env_preamble_populate(&_cmdts[0], NULL);

        vdp1_cmdt_orderlist_init(_cmdt_orderlist, VDP1_VRAM_CMDT_COUNT);

        _cmdt_orderlist[0].cmdt = &_cmdts[0];
        _cmdt_orderlist[1].cmdt = &_cmdts[1];
        _cmdt_orderlist[2].cmdt = &_cmdts[2];
        _cmdt_orderlist[3].cmdt = &_cmdts[3];

        vdp1_cmdt_orderlist_vram_patch(_cmdt_orderlist,
            (const vdp1_cmdt_t *)VDP1_VRAM(0), VDP1_VRAM_CMDT_COUNT);

        static XPDATA xpdatas_cube[1];

        sega3d_object_t object;
        object.flags = SEGA3D_OBJECT_FLAGS_WIREFRAME | SEGA3D_OBJECT_FLAGS_CULL_SCREEN | SEGA3D_OBJECT_FLAGS_CULL_AABB | SEGA3D_OBJECT_FLAGS_FOG_EXCLUDE;
        object.xpdatas = xpdatas_cube;
        object.xpdata_count = 1;

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        void *fh;
        fh = romdisk_open(romdisk, "/XPDATA.DAT");
        assert(fh != NULL);

        void * const ptr = romdisk_direct(fh);

        static sega3d_cull_aabb_t aabb;

        sega3d_s3d_t * const s3d = ptr;
        XPDATA * const xpdatas = (void *)((uintptr_t)ptr + sizeof(sega3d_s3d_t));
        sega3d_s3d_aux_t * const s3d_auxs = (void *)((uintptr_t)ptr + sizeof(sega3d_s3d_t) + (s3d->xpdata_count * sizeof(XPDATA)));

        vdp1_gouraud_table_t *gouraud_base;
        gouraud_base = _vram_partitions.gouraud_base;

        for (uint32_t j = 0; j < s3d->xpdata_count; j++) {
                /* Patch offsets */
                s3d_auxs[j].gouraud_table = (void *)((uintptr_t)s3d_auxs[j].gouraud_table + (uintptr_t)ptr);
                s3d_auxs[j].cg = (void *)((uintptr_t)s3d_auxs[j].cg + (uintptr_t)ptr);

                xpdatas[j].pntbl = (void *)((uintptr_t)xpdatas[j].pntbl + (uintptr_t)ptr);
                xpdatas[j].pltbl = (void *)((uintptr_t)xpdatas[j].pltbl + (uintptr_t)ptr);
                xpdatas[j].attbl = (void *)((uintptr_t)xpdatas[j].attbl + (uintptr_t)ptr);
                xpdatas[j].vntbl = (void *)((uintptr_t)xpdatas[j].vntbl + (uintptr_t)ptr);

                for (uint32_t i = 0; i < xpdatas[j].nbPolygon; i++) {
                        xpdatas[j].attbl[i].gstb += (uintptr_t)_vram_partitions.gouraud_base >> 3;
                }

                (void)memcpy(gouraud_base, s3d_auxs[j].gouraud_table,
                    s3d_auxs[j].gouraud_table_count * sizeof(vdp1_gouraud_table_t));

                gouraud_base += s3d_auxs[j].gouraud_table_count;
        }

        object.flags = SEGA3D_OBJECT_FLAGS_WIREFRAME |
                       SEGA3D_OBJECT_FLAGS_CULL_SCREEN |
                       SEGA3D_OBJECT_FLAGS_FOG_EXCLUDE;
        object.xpdatas = xpdatas;
        object.xpdata_count = s3d->xpdata_count;
        object.cull_shape = &aabb;

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

        rot_x[X] = toFIXED(1.0f);
        rot_y[Y] = toFIXED(1.0f);
        rot_z[Z] = toFIXED(1.0f);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                _vblanks = 0;

                sega3d_start(_cmdt_orderlist, ORDER_SEGA3D_INDEX, &_cmdts[ORDER_SEGA3D_INDEX]);

                sega3d_matrix_push(SEGA3D_MATRIX_TYPE_PUSH); {
                        /* sega3d_matrix_rot_y(rot[Y]); */
                        /* sega3d_matrix_rot_x(rot[X]); */
                        /* sega3d_matrix_rot_z(rot[Z]); */

                        fix16_t time[16];
                        fix16_t total_time = 0;
                        fix16_t *time_p = time;

                        sega3d_frustum_camera_set(camera_pos, rot_x, rot_y, rot_z);

                        for (uint32_t i = 0; i < object.xpdata_count; i++) {
                                _timing_start();

                                sega3d_object_transform(&object, i);

                                *time_p = _timing_get(); total_time += *time_p; time_p++;
                        }

                        /* for (uint32_t i = 0; i < (uint32_t)(time_p - time); i++) { */
                        /*         _timing_print(time[i]); */
                        /* } */
                        /* _timing_print(total_time); */
                } sega3d_matrix_pop();

                sega3d_finish(&results);

                if ((_digital.held.raw & PERIPHERAL_DIGITAL_A) != 0) {
                        object.flags = object.flags ^ SEGA3D_OBJECT_FLAGS_CULL_SCREEN;
                } else if ((_digital.held.raw & PERIPHERAL_DIGITAL_B) != 0) {
                        object.flags = object.flags ^ SEGA3D_OBJECT_FLAGS_WIREFRAME;
                }

                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_LEFT) != 0) {
                        camera_pos[X] -= FIX16(1.0f);
                } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_RIGHT) != 0) {
                        camera_pos[X] += FIX16(1.0f);
                }

                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_UP) != 0) {
                        camera_pos[Z] += FIX16(1.0f);
                } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_DOWN) != 0) {
                        camera_pos[Z] -= FIX16(1.0f);
                }

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
_timing_print(const fix16_t time __unused)
{
        dbgio_printf("time: %fms, ticks: %li\n", time, time);
}
