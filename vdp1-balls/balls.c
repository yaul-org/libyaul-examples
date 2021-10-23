#include <yaul.h>

#include <stdlib.h>

#include "vdp1-balls.h"

#include "balls.h"

/* #pragma GCC push_options */
/* #pragma GCC optimize ("align-functions=16") */

#define BALL_TEX_PATH   "BALL.TEX"
#define BALL_PAL_PATH   "BALL.PAL"

struct balls_handle {
        balls_config_t config;

        int8_t load_count;
};

static vdp1_vram_t _sprite_tex_base;
static vdp2_cram_t _sprite_pal_base;

static void _dma_upload(balls_handle_t *handle, const void *dst,
    const void *src, size_t len);
static void _dma_upload_wait(void);

balls_handle_t *
balls_init(const balls_config_t config)
{
        struct balls_handle * const handle =
            malloc(sizeof(struct balls_handle));
        assert(handle != NULL);

        handle->config = config;

        for (uint32_t i = 0; i < handle->config.count; i++) {
                handle->config.balls->pos_x[i] = 0;
                handle->config.balls->pos_y[i] = 0;
        }

        return handle;
}

void
balls_assets_init(balls_handle_t *handle __unused)
{
        vdp1_vram_partitions_t vdp1_vram_partitions;

        vdp1_vram_partitions_get(&vdp1_vram_partitions);

        _sprite_tex_base = (vdp1_vram_t)vdp1_vram_partitions.texture_base;
        _sprite_pal_base = VDP2_CRAM_MODE_1_OFFSET(0, 1, 0x0000);
}

void
balls_assets_load(balls_handle_t *handle)
{
        void *fh[2];
        void *p;
        size_t len;

        handle->load_count = 2;

        fh[0] = romdisk_open(_romdisk, BALL_TEX_PATH);
        assert(fh[0] != NULL);
        p = romdisk_direct(fh[0]);
        len = romdisk_total(fh[0]);
        _dma_upload(handle, (void *)_sprite_tex_base, p, len);
        romdisk_close(fh[0]);

        fh[1] = romdisk_open(_romdisk, BALL_PAL_PATH);
        assert(fh[1] != NULL);
        p = romdisk_direct(fh[1]);
        len = romdisk_total(fh[1]);
        _dma_upload(handle, (void *)_sprite_pal_base, p, len);
        romdisk_close(fh[1]);

        _dma_upload_wait();
}

static inline q0_12_4_t
_ball_position_update(q0_12_4_t pos, q0_12_4_t speed)
{
        /* Map bit 0 as direction:
         *   dir_bit: 0 -> ((0-1)^0xFF)=0x00 -> positive
         *   dir_bit: 1 -> ((1-1)^0xFF)=0xFF -> negative */
        const int8_t dir_bit = pos & 0x0001;
        const uint8_t d = (dir_bit - 1) ^ 0xFF;
        const q0_12_4_t fixed_dir = (q0_12_4_t)((int8_t)d ^ speed);

        return (pos + fixed_dir) | dir_bit;
}

void
balls_position_update(balls_handle_t *handle, uint32_t count)
{
        const balls_t * const balls = handle->config.balls;

        q0_12_4_t *pos_x = balls->pos_x;
        q0_12_4_t *pos_y = balls->pos_y;

        const uint32_t unrolled_count = count / 8;
        const uint32_t remainder_count = count & 7;
        const q0_12_4_t speed = handle->config.speed;

        for (uint32_t i = 0; i < unrolled_count; i++) {
                *pos_x = _ball_position_update(*pos_x, speed); pos_x++;
                *pos_x = _ball_position_update(*pos_x, speed); pos_x++;
                *pos_x = _ball_position_update(*pos_x, speed); pos_x++;
                *pos_x = _ball_position_update(*pos_x, speed); pos_x++;
                *pos_x = _ball_position_update(*pos_x, speed); pos_x++;
                *pos_x = _ball_position_update(*pos_x, speed); pos_x++;
                *pos_x = _ball_position_update(*pos_x, speed); pos_x++;
                *pos_x = _ball_position_update(*pos_x, speed); pos_x++;
        }

        for (uint32_t i = 0; i < remainder_count; i++) {
                *pos_x = _ball_position_update(*pos_x, speed); pos_x++;
        }

        for (uint32_t i = 0; i < unrolled_count; i++) {
                *pos_y = _ball_position_update(*pos_y, speed); pos_y++;
                *pos_y = _ball_position_update(*pos_y, speed); pos_y++;
                *pos_y = _ball_position_update(*pos_y, speed); pos_y++;
                *pos_y = _ball_position_update(*pos_y, speed); pos_y++;
                *pos_y = _ball_position_update(*pos_y, speed); pos_y++;
                *pos_y = _ball_position_update(*pos_y, speed); pos_y++;
                *pos_y = _ball_position_update(*pos_y, speed); pos_y++;
                *pos_y = _ball_position_update(*pos_y, speed); pos_y++;
        }


        for (uint32_t i = 0; i < remainder_count; i++) {
                *pos_y = _ball_position_update(*pos_y, speed); pos_y++;
        }
}

void
balls_position_clamp(balls_handle_t *handle, uint16_t count)
{
        const balls_t * const balls = handle->config.balls;

        const q0_12_4_t speed = handle->config.speed;

        const q0_12_4_t left_clamp = -SCREEN_HWIDTH_Q;
        const q0_12_4_t right_clamp = SCREEN_HWIDTH_Q;

        q0_12_4_t *pos_x = balls->pos_x;

        for (uint32_t i = 0; i < count; i++) {
                if (*pos_x <= left_clamp) {
                        *pos_x = (left_clamp + speed) & ~0x0001;
                } else if (*pos_x > right_clamp) {
                        *pos_x = (right_clamp - speed) | 0x0001;
                }

                pos_x++;
        }

        const q0_12_4_t up_clamp = -SCREEN_HHEIGHT_Q;
        const q0_12_4_t down_clamp = SCREEN_HHEIGHT_Q;

        q0_12_4_t *pos_y = balls->pos_y;

        for (uint32_t i = 0; i < count; i++) {
                if (*pos_y < up_clamp) {
                        *pos_y = (up_clamp + speed) & ~0x0001;
                } else if (*pos_y > down_clamp) {
                        *pos_y = (down_clamp - speed) | 0x0001;
                }

                pos_y++;
        }
}

void
balls_cmdts_put(balls_handle_t *handle __unused, uint16_t index, uint16_t count)
{
        const vdp1_cmdt_draw_mode_t draw_mode = {
                .raw                       = 0x0000,
                .bits.color_mode           = 0,
                .bits.trans_pixel_disable  = false,
                .bits.pre_clipping_disable = true,
                .bits.end_code_disable     = false
        };

        const uint16_t offset =
            (uint32_t)_sprite_pal_base & (VDP2_CRAM_SIZE - 1);
        const uint16_t palette_number = offset >> 1;

        const vdp1_cmdt_color_bank_t color_bank ={
                .raw = 0x0000,
                .type_0.data.dc = palette_number & VDP2_SPRITE_TYPE_0_DC_MASK
        };

        vdp1_cmdt_t *cmdt;
        cmdt = (vdp1_cmdt_t *)VDP1_CMD_TABLE(index, 0);

        for (uint32_t i = 0; i < count; i++) {
                vdp1_cmdt_normal_sprite_set(cmdt);

                vdp1_cmdt_param_draw_mode_set(cmdt, draw_mode);
                vdp1_cmdt_param_color_mode0_set(cmdt, color_bank);
                vdp1_cmdt_param_size_set(cmdt, BALL_WIDTH, BALL_HEIGHT);
                vdp1_cmdt_param_char_base_set(cmdt, _sprite_tex_base);

                cmdt->cmd_xa = 0;
                cmdt->cmd_ya = 0;

                cmdt++;
        }

        vdp1_cmdt_end_set(cmdt);
}

void
balls_cmdts_position_put(balls_handle_t *handle, uint16_t index, uint16_t count)
{
        const balls_t * const balls = handle->config.balls;

        vdp1_sync_cmdt_stride_put(balls->cmd_xa,
            count,
            6, /* CMDXA */
            index);

        vdp1_sync_cmdt_stride_put(balls->cmd_ya,
            count,
            7, /* CMDYA */
            index);
}

void
balls_cmdts_update(balls_handle_t *handle, uint16_t count)
{
        const balls_t * const balls = handle->config.balls;

        q0_12_4_t *pos_x = balls->pos_x;
        q0_12_4_t *pos_y = balls->pos_y;

        int16_t *cmd_xa = balls->cmd_xa;
        int16_t *cmd_ya = balls->cmd_ya;

        for (uint32_t i = 0; i < count; i++) {
                *cmd_xa = Q0_12_4_INT(*pos_x);
                pos_x++;
                cmd_xa++;
        }

        for (uint32_t i = 0; i < count; i++) {
                *cmd_ya = Q0_12_4_INT(*pos_y);
                pos_y++;
                cmd_ya++;
        }
}

static void
_dma_upload(balls_handle_t *handle __unused, const void *dst, const void *src, size_t len)
{
        const struct scu_dma_level_cfg scu_dma_level_cfg = {
                .mode            = SCU_DMA_MODE_DIRECT,
                .space           = SCU_DMA_SPACE_BUS_B,
                .stride          = SCU_DMA_STRIDE_2_BYTES,
                .update          = SCU_DMA_UPDATE_NONE,
                .xfer.direct.len = len,
                .xfer.direct.dst = (uint32_t)dst,
                .xfer.direct.src = CPU_CACHE_THROUGH | (uint32_t)src
        };

        struct scu_dma_handle dma_handle;

        scu_dma_config_buffer(&dma_handle, &scu_dma_level_cfg);

        int8_t ret;
        ret = dma_queue_enqueue(&dma_handle, DMA_QUEUE_TAG_IMMEDIATE, NULL, NULL);
        assert(ret == 0);
}

static void
_dma_upload_wait(void)
{
        dma_queue_flush(DMA_QUEUE_TAG_IMMEDIATE);
        dma_queue_flush_wait();
}

/* #pragma GCC pop_options */
