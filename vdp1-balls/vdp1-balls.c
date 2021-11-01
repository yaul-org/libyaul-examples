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

#include "perf.h"

#include "balls.h"

#include "q0_12_4.h"

#define VDP1_VRAM_CMDT_COUNT    (BALL_MAX_COUNT + 3)
#define VDP1_VRAM_TEXTURE_SIZE  (0x0005BF60)
#define VDP1_VRAM_GOURAUD_COUNT (1024)
#define VDP1_VRAM_CLUT_COUNT    (256)

#define VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX  (0)
#define VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX        (1)
#define VDP1_CMDT_ORDER_BALL_START_INDEX          (2)

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

static q0_12_4_t _balls_pos_x[BALL_MAX_COUNT] __aligned(0x1000);
static q0_12_4_t _balls_pos_y[BALL_MAX_COUNT] __aligned(0x1000);

static int16_t _balls_cmd_xa[2][BALL_MAX_COUNT] __aligned(0x1000);
static int16_t _balls_cmd_ya[2][BALL_MAX_COUNT] __aligned(0x1000);

static volatile uint32_t _transfer_over_count = 0;

struct buffer_context {
        balls_handle_t *balls_handle;
        vdp1_cmdt_t *cmdt_draw_end;
};

static void _romdisk_init(void);

static void _vdp1_init(void);
static void _vdp2_init(void);

static void _vblank_out_handler(void *work);

static void _transfer_over(void *work);

int
main(void)
{
        const balls_t balls[] = {
                {
                        .pos_x  = _balls_pos_x,
                        .pos_y  = _balls_pos_y,
                        .cmd_xa = _balls_cmd_xa[0],
                        .cmd_ya = _balls_cmd_ya[0]
                }, {
                        .pos_x  = _balls_pos_x,
                        .pos_y  = _balls_pos_y,
                        .cmd_xa = _balls_cmd_xa[1],
                        .cmd_ya = _balls_cmd_ya[1]
                }
        };

        const balls_config_t _balls_configs[] = {
                {
                        .balls = &balls[0],
                        .count = BALL_MAX_COUNT,
                        .speed = BALL_SPEED
                }, {
                        .balls = &balls[1],
                        .count = BALL_MAX_COUNT,
                        .speed = BALL_SPEED
                }
        };

        const uint8_t sync_modes[] = {
                VDP1_SYNC_MODE_ERASE_CHANGE,
                VDP1_SYNC_MODE_CHANGE_ONLY
        };

        uint32_t which_context;
        which_context = 0;

        uint8_t sync_mode;
        sync_mode = 0;

        uint32_t balls_count;
        balls_count = 1;

        balls_handle_t *balls_handle[2];
        smpc_peripheral_digital_t digital;

        balls_handle[0] = balls_init(_balls_configs[0]);
        balls_handle[1] = balls_init(_balls_configs[1]);

        struct buffer_context buffer_contexts[] = {
                {
                        .balls_handle  = balls_handle[0],
                        .cmdt_draw_end = (vdp1_cmdt_t *)VDP1_CMD_TABLE(VDP1_CMDT_ORDER_BALL_START_INDEX, 0),
                }, {
                        .balls_handle  = balls_handle[1],
                        .cmdt_draw_end = NULL,
                }
        };

        balls_assets_init(balls_handle[0]);
        balls_assets_load(balls_handle[0]);

        /* Write to VRAM directly */
        balls_cmdts_put(balls_handle[0], VDP1_CMDT_ORDER_BALL_START_INDEX, BALL_MAX_COUNT);

        struct buffer_context * previous_buffer_context = &buffer_contexts[0];
        struct buffer_context * buffer_context = &buffer_contexts[0];

        perf_t cpu_perf;
        perf_t vdp1_perf;
        perf_t dma_perf;

        perf_init(&cpu_perf);
        perf_init(&vdp1_perf);
        perf_init(&dma_perf);

        vdp1_sync_transfer_over_set(_transfer_over, NULL);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &digital);

                if ((digital.held.button.a != 0) || (digital.pressed.button.b != 0)) {
                        balls_count++;
                }

                if (digital.held.button.y != 0) {
                        balls_count += 256;
                }

                if (digital.held.button.start != 0) {
                        smpc_smc_sysres_call();

                        return 0;
                }

                if (digital.held.button.x != 0) {
                        sync_mode ^= 1;

                        vdp1_sync_mode_set(sync_modes[sync_mode]);
                }

                if (digital.held.button.c != 0) {
                        balls_count = 1;
                }

                if (balls_count >= BALL_MAX_COUNT) {
                        balls_count = BALL_MAX_COUNT;
                }

                perf_start(&cpu_perf); {
                        balls_position_update(buffer_context->balls_handle, balls_count);
                        balls_position_clamp(buffer_context->balls_handle, balls_count);

                        balls_cmdts_update(buffer_context->balls_handle, balls_count);
                } perf_end(&cpu_perf);

                /* Wait for the previous sync (if any) */
                vdp1_sync_wait();

                perf_start(&dma_perf); {
                        balls_cmdts_position_put(buffer_context->balls_handle, VDP1_CMDT_ORDER_BALL_START_INDEX, balls_count);
                } perf_end(&dma_perf);

                vdp1_cmdt_end_clear(previous_buffer_context->cmdt_draw_end);

                buffer_context->cmdt_draw_end =
                    (vdp1_cmdt_t *)VDP1_CMD_TABLE(VDP1_CMDT_ORDER_BALL_START_INDEX + balls_count, 0);

                vdp1_cmdt_end_set(buffer_context->cmdt_draw_end);

                /* Call to render */
                vdp1_sync_render();
                /* Call to sync with frame change -- does not block */
                vdp1_sync();

                which_context ^= 1;
                previous_buffer_context = buffer_context;
                buffer_context = &buffer_contexts[which_context];

                dbgio_printf("[H[2J"
                             "ball_count: %4lu, which: %lu\n"
                             " CPU: %7lu (max: %7lu)\n"
                             " DMA: %7lu (max: %7lu)\n"
                             "VDP1: %7lu (max: %7lu)\n"
                             "\n"
                             "Transfer-over: %i\n",
                    balls_count,
                    which_context,
                    cpu_perf.ticks,
                    cpu_perf.max_ticks,
                    dma_perf.ticks,
                    dma_perf.max_ticks,
                    vdp1_perf.ticks,
                    vdp1_perf.max_ticks,
                    _transfer_over_count);

                dbgio_flush();

                vdp2_sync();
        }

        return 0;
}

void
user_init(void)
{
        _romdisk_init();

        _vdp2_init();
        _vdp1_init();

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        vdp2_sync();
        vdp2_sync_wait();

        perf_system_init();
}

static void
_romdisk_init(void)
{
        romdisk_init();

        _romdisk = romdisk_mount(root_romdisk);
        assert(_romdisk != NULL);
}

static void
_vdp1_init(void)
{
        const struct vdp1_env vdp1_env = {
                .bpp          = VDP1_ENV_BPP_16,
                .rotation     = VDP1_ENV_ROTATION_0,
                .color_mode   = VDP1_ENV_COLOR_MODE_PALETTE,
                .sprite_type  = 0,
                .erase_color  = COLOR_RGB1555(0, 0, 0, 0),
                .erase_points = {
                        {                0,                 0 },
                        { RESOLUTION_WIDTH, RESOLUTION_HEIGHT }
                }
        };

        vdp2_sprite_priority_set(0, 6);


        const int16_vec2_t local_coords =
            INT16_VEC2_INITIALIZER((RESOLUTION_WIDTH / 2) - BALL_HWIDTH - 1,
                                   (RESOLUTION_HEIGHT / 2) - BALL_HHEIGHT - 1);

        const int16_vec2_t system_clip_coords =
            INT16_VEC2_INITIALIZER(RESOLUTION_WIDTH,
                                   RESOLUTION_HEIGHT);

        /* Write directly to VDP1 VRAM. Since the first two command tables never
         * change, we can write directly */
        vdp1_cmdt_t * const cmdt = (vdp1_cmdt_t *)VDP1_CMD_TABLE(0, 0);

        vdp1_cmdt_system_clip_coord_set(&cmdt[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdt[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX],
            CMDT_VTX_SYSTEM_CLIP, &system_clip_coords);

        vdp1_cmdt_local_coord_set(&cmdt[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdt[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX],
            CMDT_VTX_LOCAL_COORD, &local_coords);


        vdp1_env_set(&vdp1_env);

        vdp1_vram_partitions_set(VDP1_VRAM_CMDT_COUNT,
            VDP1_VRAM_TEXTURE_SIZE,
            VDP1_VRAM_GOURAUD_COUNT,
            VDP1_VRAM_CLUT_COUNT);

        vdp1_sync_interval_set(0);
}

static void
_vdp2_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_B,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp2_tvmd_display_set();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}

static void
_transfer_over(void *work __unused)
{
        _transfer_over_count++;
}
