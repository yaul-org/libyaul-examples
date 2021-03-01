/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#include "vdp1-balls.h"
#include "balls.h"
#include "q0_12_4.h"

#define ORDER_SYSTEM_CLIP_COORDS_INDEX  (0)
#define ORDER_LOCAL_COORDS_INDEX        (1)
#define ORDER_BALL_START_INDEX          (2)
#define ORDER_DRAW_END_INDEX            (ORDER_BALL_START_INDEX + BALL_MAX_COUNT)
#define ORDER_COUNT                     (ORDER_DRAW_END_INDEX + 1)

/* Possible values:
 *   0x1: 0.062500000 -> Ignored
 *   0x2: 0.125000000
 *   0x3: 0.187500000 -> Ignored
 *   0x4: 0.250000000
 *   0x5: 0.312500000 -> Ignored
 *   0x6: 0.375000000
 *   0x7: 0.437500000 -> Ignored
 *   0x8: 0.500000000
 *   0x9: 0.562500000 -> Ignored
 *   0xA: 0.625000000
 *   0xB: 0.687500000 -> Ignored
 *   0xC: 0.750000000
 *   0xD: 0.812500000 -> Ignored
 *   0xE: 0.875000000
 *   0xF: 0.937500000 -> Ignored */

#define BALL_SPEED (0x000E)

extern uint8_t root_romdisk[];

void *_romdisk;

static struct ball _balls[BALL_MAX_COUNT] __aligned(0x1000);
static struct balls_config _balls_config;

static balls_handle_t _balls_handle;

static vdp1_cmdt_list_t *_cmdt_list;

static smpc_peripheral_digital_t _digital;

static void _romdisk_init(void);
static void _cmdt_list_init(void);

static void _vblank_out_handler(void *);
static void _cpu_frt_ovi_handler(void);

int
main(void)
{
        _romdisk_init();
        _cmdt_list_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        vdp1_cmdt_t *cmdts;
        cmdts = &_cmdt_list->cmdts[ORDER_BALL_START_INDEX];

        vdp1_vram_partitions_t vdp1_vram_partitions;

        vdp1_vram_partitions_get(&vdp1_vram_partitions);

        void *tex_base;
        tex_base = vdp1_vram_partitions.texture_base;
        void *pal_base;
        pal_base = (void *)VDP2_CRAM_MODE_1_OFFSET(0, 1, 0x0000);

        _balls_config.balls = _balls;
        _balls_config.count = BALL_MAX_COUNT;
        _balls_config.sprite_tex_base = tex_base;
        _balls_config.sprite_pal_base = pal_base;
        _balls_config.dma_tag = DMA_QUEUE_TAG_IMMEDIATE;

        _balls_handle = balls_init(&_balls_config);

        balls_sprite_load(_balls_handle, NULL);

        dma_queue_flush(DMA_QUEUE_TAG_IMMEDIATE);

        for (uint32_t i = 0; i < _balls_config.count; i++) {
                _balls[i].pos_x = 0;
                _balls[i].pos_y = 0;
                _balls[i].speed = BALL_SPEED;
        }

        balls_cmdt_list_init(_balls_handle, cmdts, BALL_MAX_COUNT);

        dma_queue_flush_wait();

        const uint8_t sync_modes[] = {
                VDP1_SYNC_MODE_ERASE_CHANGE,
                VDP1_SYNC_MODE_CHANGE_ONLY
        };

        uint8_t sync_mode;
        sync_mode = 0;

        uint32_t ball_count;
        ball_count = 1;

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                if (((_digital.held.button.a) != 0) || ((_digital.pressed.button.b) != 0)) {
                        ball_count++;

                        if (ball_count >= BALL_MAX_COUNT) {
                                ball_count = BALL_MAX_COUNT;
                        }
                }

                if ((_digital.held.button.x) != 0) {
                        sync_mode ^= 1;

                        vdp1_sync_mode_set(sync_modes[sync_mode]);
                }

                if ((_digital.held.button.c) != 0) {
                        ball_count = 1;
                }

                balls_position_update(_balls_handle, ball_count);
                balls_position_clamp(_balls_handle, ball_count);
                balls_cmdt_list_update(_balls_handle, cmdts, ball_count);

                /* End the command table list */
                vdp1_cmdt_end_set(&cmdts[ball_count]);

                _cmdt_list->count = 2 + ball_count + 1;

                vdp1_sync_cmdt_list_put(_cmdt_list, 0, NULL, NULL);

                dbgio_printf("[H[2J"
                             "ball_count: %lu\n",
                             ball_count);

                dbgio_flush();

                vdp2_tvmd_vblank_in_wait();
                vdp_sync();
        }

        return 0;
}

void
user_init(void)
{
        static const struct vdp1_env vdp1_env = {
                .bpp = VDP1_ENV_BPP_16,
                .rotation = VDP1_ENV_ROTATION_0,
                .color_mode = VDP1_ENV_COLOR_MODE_RGB_PALETTE,
                .sprite_type = 0,
                .erase_color = COLOR_RGB1555(0, 0, 0, 0),
                .erase_points = {
                        {                0,                 0 },
                        { RESOLUTION_WIDTH, RESOLUTION_HEIGHT }
                }
        };

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_B,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp2_sprite_priority_set(0, 6);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();

        vdp1_env_set(&vdp1_env);

        vdp_sync_vblank_out_set(_vblank_out_handler);

        cpu_frt_init(CPU_FRT_CLOCK_DIV_32);
        cpu_frt_ovi_set(_cpu_frt_ovi_handler);
}

static void
_romdisk_init(void)
{
        romdisk_init();

        _romdisk = romdisk_mount("/", root_romdisk);
        assert(_romdisk != NULL);
}

static void
_cmdt_list_init(void)
{
        _cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);

        vdp1_cmdt_t *cmdt;
        cmdt = &_cmdt_list->cmdts[0];

        static const int16_vec2_t local_coords =
            INT16_VEC2_INITIALIZER(RESOLUTION_WIDTH / 2,
                RESOLUTION_HEIGHT / 2);

        static const int16_vec2_t system_clip_coords =
            INT16_VEC2_INITIALIZER(RESOLUTION_WIDTH,
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

static void
_cpu_frt_ovi_handler(void)
{
}
