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

extern XPDATA PD_CUBE1[];

extern Uint16 GR_SMS[];
extern XPDATA PD_SMS3[];

extern TEXTURE TEX_SAMPLE[];
extern PICTURE PIC_SAMPLE[];

extern uint8_t root_romdisk[];

static void _vblank_in_handler(void *);
static void _vblank_out_handler(void *);
static void _frt_ovi_handler(void);

static void _timing_start(void) __used;
static fix16_t _timing_get(void) __used;
static void _timing_print(const fix16_t time) __used;
static void _assets_copy(const sega3d_object_t *object) __used;

static smpc_peripheral_digital_t _digital;

static volatile uint16_t _vblanks = 0;
static volatile uint16_t _frt_ovf_count = 0;

static vdp1_cmdt_orderlist_t _cmdt_orderlist[VDP1_VRAM_CMDT_COUNT] __aligned(0x20000);
static vdp1_cmdt_t *_cmdts;

static TEXTURE _textures[64];
/* static PALETTE _palettes[64]; */

static vdp1_vram_partitions_t _vram_partitions;

static void
_divu_ovfi_handler(void)
{
}

static void
_dma_illegal_handler(void)
{
}

static const POINT _origins[] __unused = {
        /* POStoFIXED(0.0f, 0.0f, 0.0f) */
        POStoFIXED(-327.74971146245036,    -99.7805595454549,     -25.0098638142293),
        POStoFIXED( 318.41509807355493,    -99.33722744308275,    -18.493731033274976),
        POStoFIXED(   1.7962270085470124,  -65.39568931623924,    278.1383482905985),
        POStoFIXED(   0.8227719548872167,    2.4680098872181877, -406.55613283208095),
        POStoFIXED( -25.937490530973417,  -113.62659061946981,    -48.8466574778761)
};

static const FIXED _radii[] __unused = {
        /* toFIXED(34.64101615137755f * 1.1), */
        toFIXED(247.93746824610838 * 1.1),
        toFIXED(257.88756123558966 * 1.1),
        toFIXED(248.99245032794167 * 1.1),
        toFIXED(397.2539339012265  * 1.1),
        toFIXED(336.20933973982636 * 1.1)
};

static void __unused
_ztp_tex_fn(sega3d_ztp_handle_t *handle __unused, const sega3d_ztp_tex_t *tex)
{
        const TEXTURE * const texture = tex->texture;
        void *vram = (void *)(VDP1_VRAM(0) | (texture->CGadr << 3));

        dma_queue_simple_enqueue(DMA_QUEUE_TAG_IMMEDIATE, vram, tex->offset_cg, tex->cg_size);

        void *palette = (void *)(VDP1_VRAM(0) | (tex->palette_num << 3));
        dma_queue_simple_enqueue(DMA_QUEUE_TAG_IMMEDIATE, palette, tex->offset_palette, tex->palette_size);

        dma_queue_flush(DMA_QUEUE_TAG_IMMEDIATE);
}

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
        /* sega3d_plist_set(_palettes, 64); */

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

        static XPDATA xpdatas_cube[1];

        (void)memcpy(&xpdatas_cube[0], PD_CUBE1, sizeof(XPDATA));

        sega3d_object_t object;
        object.flags = SEGA3D_OBJECT_FLAGS_WIREFRAME | SEGA3D_OBJECT_FLAGS_CULL_SCREEN | SEGA3D_OBJECT_FLAGS_CULL_AABB | SEGA3D_OBJECT_FLAGS_FOG_EXCLUDE;
        object.xpdatas = xpdatas_cube;
        object.xpdata_count = 1;

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);
#if ZTP
        static XPDATA xpdatas[1];
        void *fh;
        fh = romdisk_open(romdisk, "/SONIC2.ZTP");
        assert(fh != NULL);

        void * const ptr = romdisk_direct(fh);
        sega3d_ztp_t ztp __unused;

        static ATTR attrs[512];
        static sega3d_cull_aabb_t ztp_aabb;

        ztp.data = ptr;
        ztp.flags = SEGA3D_ZTP_FLAG_USE_AABB;
        ztp.xpdatas = &xpdatas[0];
        ztp.attrs = attrs;
        ztp.texture_num = 0;
        ztp.palette_num = (uint32_t)_vram_partitions.clut_base >> 3; 
        ztp.cull_shape = &ztp_aabb;

        sega3d_ztp_handle_t handle;
        handle = sega3d_ztp_parse(&object, &ztp);

        sega3d_ztp_textures_parse(&handle, _vram_partitions.texture_base, _ztp_tex_fn);

        for (uint32_t i = 0; i < xpdatas[0].nbPolygon; i++) {
                ATTR * const attr __unused = &xpdatas[0].attbl[i];
                /* dbgio_printf("attr->sort: 0x%02X\n", attr->sort & 0x3); */
        }

        dma_queue_flush(DMA_QUEUE_TAG_IMMEDIATE);
        dma_queue_flush_wait();
#else
        void *fh;
        fh = romdisk_open(romdisk, "/XPDATA.DAT");
        assert(fh != NULL);

        void * const ptr = romdisk_direct(fh);

        static sega3d_cull_aabb_t aabb;

        sega3d_s3d_t * const s3d = ptr;
        XPDATA * const xpdatas = (void *)((uintptr_t)ptr + sizeof(sega3d_s3d_t));
        sega3d_s3d_aux_t * const s3d_auxs = (void *)((uintptr_t)ptr + sizeof(sega3d_s3d_t) + (s3d->xpdata_count * sizeof(XPDATA)));

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

                (void)memcpy(_vram_partitions.gouraud_base + (j * (s3d_auxs[j].gouraud_table_count * 8)),
                    s3d_auxs[j].gouraud_table,
                    s3d_auxs[j].gouraud_table_count * 8);
        }

        object.flags = SEGA3D_OBJECT_FLAGS_WIREFRAME | SEGA3D_OBJECT_FLAGS_CULL_SCREEN | SEGA3D_OBJECT_FLAGS_FOG_EXCLUDE;
        object.xpdatas = xpdatas;
        object.xpdata_count = s3d->xpdata_count;
        object.cull_shape = &aabb;
#endif

        /* _assets_copy(&object); */

        ANGLE rot[XYZ] __unused;
        rot[X] = DEGtoANG(0.0f);
        rot[Y] = DEGtoANG(0.0f);
        rot[Z] = DEGtoANG(0.0f);

        sega3d_results_t results __unused;

        POINT camera_pos;
        VECTOR rot_x;
        VECTOR rot_y;
        VECTOR rot_z;
        camera_pos[X] = camera_pos[Y] = camera_pos[Z] = toFIXED(0.0f);
        rot_x[X] = rot_x[Y] = rot_x[Z] = toFIXED(0.0f);
        rot_y[X] = rot_y[Y] = rot_y[Z] = toFIXED(0.0f);
        rot_z[X] = rot_z[Y] = rot_z[Z] = toFIXED(0.0f);

        rot_x[X] = toFIXED(1.0f);
        rot_y[Y] = toFIXED(1.0f);
        rot_z[Z] = toFIXED(1.0f);
        camera_pos[Z] = toFIXED(-100.0f);

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

                /* dbgio_printf("_vblanks: %u, object/polygon: %lu/%lu, 0x%02X, (%f,%f,%f)\n", */
                /*     _vblanks, */
                /*     results.object_count, */
                /*     results.polygon_count, */
                /*     object.flags, */
                /*     camera_pos[X], */
                /*     camera_pos[Y], */
                /*     camera_pos[Z]); */
                /* dbgio_flush(); */
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
        /* dbgio_flush(); */
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

static void
_assets_copy(const sega3d_object_t *object __unused)
{
        /* const uint16_t polygon_count __unused = */
        /*     sega3d_object_polycount_get(object); */

        /* (void)memcpy((uint16_t *)VDP1_VRAM(0x2BFE0), */
        /*     GR_SMS, */
        /*     sizeof(vdp1_gouraud_table_t) * polygon_count); */

        /* vdp1_vram_partitions_t vdp1_vram_parts; */
        /* vdp1_vram_partitions_get(&vdp1_vram_parts); */

        /* for (uint32_t i = 0; i < 2; i++) { */
        /*         const PICTURE *picture; */
        /*         picture = &PIC_SAMPLE[i]; */
        /*         const TEXTURE *texture; */
        /*         texture = &TEX_SAMPLE[picture->texno]; */

        /*         const uint32_t vram_ptr = VDP1_VRAM(texture->CGadr << 3); */

        /*         const uint32_t size = */
        /*             (texture->Hsize * texture->Vsize * 4) >> picture->cmode; */

        /*         (void)memcpy((void *)vram_ptr, picture->pcsrc, size); */
        /* } */
}
