#include <yaul.h>

#include <stdlib.h>

#include "vdp1-balls.h"

#include "balls.h"

#define BALL_TEX_PATH   "BALL.TEX"
#define BALL_PAL_PATH   "BALL.PAL"

#define BALL_WIDTH      (16)
#define BALL_HEIGHT     (16)

#define BALL_HWIDTH     (BALL_WIDTH / 2)
#define BALL_HHEIGHT    (BALL_HEIGHT / 2)

struct balls_handle {
        struct balls_config config;

        balls_load_callback load_callback;
        int8_t load_count;
};

static void _dma_upload(balls_handle_t handle, const void *dst,
    const void *src, size_t len);
static void _dma_upload_handler(const dma_queue_transfer_t *transfer);

balls_handle_t
balls_init(const struct balls_config *config)
{
        struct balls_handle *handle;
        handle = malloc(sizeof(struct balls_handle));

        (void)memcpy(&handle->config, config, sizeof(struct balls_config));

        (void)memset(&handle->config.balls[0], 0x00,
            handle->config.count * sizeof(struct ball));

        return handle;
}

void
balls_sprite_load(balls_handle_t handle, balls_load_callback callback)
{
        void *fh[2];
        void *p;
        size_t len;

        handle->load_callback = callback;
        handle->load_count = 2;

        fh[0] = romdisk_open(_romdisk, BALL_TEX_PATH);
        assert(fh[0] != NULL);
        p = romdisk_direct(fh[0]);
        len = romdisk_total(fh[0]);

        _dma_upload(handle, handle->config.sprite_tex_base, p, len);

        fh[1] = romdisk_open(_romdisk, BALL_PAL_PATH);
        assert(fh[1] != NULL);
        p = romdisk_direct(fh[1]);
        len = romdisk_total(fh[1]);
        _dma_upload(handle, handle->config.sprite_pal_base, p, len);

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);
}

void
balls_position_update(balls_handle_t handle, const uint16_t count)
{
        for (uint16_t i = 0; i < count; i++) {
                struct ball *ball = &handle->config.balls[i];

                /* Map bit 0 as direction:
                 *   dir_?_bit: 0 -> ((0-1)^0xFF)=0x00 -> positive
                 *   dir_?_bit: 1 -> ((1-1)^0xFF)=0xFF -> negative */
                const int8_t dir_x_bit = ball->pos_x & 0x0001;
                const uint8_t d_x = (dir_x_bit - 1) ^ 0xFF;
                const q0_12_4_t fixed_dir_x = (q0_12_4_t)((int8_t)d_x ^ ball->speed);

                const int8_t dir_y_bit = ball->pos_y & 0x0001;
                const uint8_t d_y = (dir_y_bit - 1) ^ 0xFF;
                const q0_12_4_t fixed_dir_y = (q0_12_4_t)((int8_t)d_y ^ ball->speed);

                ball->pos_x = (ball->pos_x + fixed_dir_x) | dir_x_bit;
                ball->pos_y = (ball->pos_y + fixed_dir_y) | dir_y_bit;
        }
}

void
balls_position_clamp(balls_handle_t handle, const uint16_t count)
{
        const q0_12_4_t left_clamp = -SCREEN_HWIDTH_Q;
        const q0_12_4_t right_clamp = SCREEN_HWIDTH_Q;

        for (uint16_t i = 0; i < count; i++) {
                struct ball *ball = &handle->config.balls[i];

                if (ball->pos_x <= left_clamp) {
                        ball->pos_x = (left_clamp + ball->speed) & ~0x0001;
                } else if (ball->pos_x > right_clamp) {
                        ball->pos_x = (right_clamp - ball->speed) | 0x0001;
                }

                if (ball->pos_y < (-SCREEN_HHEIGHT_Q)) {
                        ball->pos_y = ((-SCREEN_HHEIGHT_Q) + ball->speed) & ~0x0001;
                } else if (ball->pos_y > SCREEN_HHEIGHT_Q) {
                        ball->pos_y = (SCREEN_HHEIGHT_Q - ball->speed) | 0x0001;
                }
        }
}

void
balls_cmdt_list_init(balls_handle_t handle, vdp1_cmdt_t *cmdts, const uint16_t count)
{
        static const vdp1_cmdt_draw_mode_t draw_mode = {
                .raw = 0x0000,
                .bits.color_mode = 0,
                .bits.trans_pixel_disable = false,
                .bits.pre_clipping_disable =true,
                .bits.end_code_disable = true
        };

        vdp1_cmdt_color_bank_t color_bank;

        const uint16_t offset =
            (uint32_t)handle->config.sprite_pal_base & (VDP2_CRAM_SIZE - 1);
        const uint16_t palette_number = offset >> 1;

        color_bank.raw = 0x0000;
        color_bank.type_0.data.dc = palette_number & VDP2_SPRITE_TYPE_0_DC_MASK;

        for (uint16_t i = 0; i < count; i++) {
                vdp1_cmdt_t *cmdt;
                cmdt = &cmdts[i];

                vdp1_cmdt_normal_sprite_set(cmdt);

                vdp1_cmdt_param_draw_mode_set(cmdt, draw_mode);
                vdp1_cmdt_param_color_mode0_set(cmdt, color_bank);
                vdp1_cmdt_param_size_set(cmdt, BALL_WIDTH, BALL_HEIGHT);
                vdp1_cmdt_param_char_base_set(cmdt,
                    (uint32_t)handle->config.sprite_tex_base);

                cmdt->cmd_xa = 0;
                cmdt->cmd_ya = 0;
        }

        vdp1_cmdt_end_set(&cmdts[count]);
}

void
balls_cmdt_list_update(balls_handle_t handle, vdp1_cmdt_t *cmdts, const uint16_t count)
{
        for (uint16_t i = 0; i < count; i++) {
                struct ball *ball = &handle->config.balls[i];

                vdp1_cmdt_t *cmdt;
                cmdt = &cmdts[i];

                vdp1_cmdt_normal_sprite_set(cmdt);

                cmdt->cmd_xa = Q0_12_4_INT(ball->pos_x) - BALL_HWIDTH - 1;
                cmdt->cmd_ya = Q0_12_4_INT(ball->pos_y) - BALL_HHEIGHT - 1;
        }
}

static void
_dma_upload(balls_handle_t handle, const void *dst, const void *src, size_t len)
{
        const struct scu_dma_level_cfg scu_dma_level_cfg = {
                .mode = SCU_DMA_MODE_DIRECT,
                .stride = SCU_DMA_STRIDE_2_BYTES,
                .update = SCU_DMA_UPDATE_NONE,
                .xfer.direct.len = len,
                .xfer.direct.dst = (uint32_t)dst,
                .xfer.direct.src = CPU_CACHE_THROUGH | (uint32_t)src
        };

        struct scu_dma_handle dma_handle;

        scu_dma_config_buffer(&dma_handle, &scu_dma_level_cfg);

        int8_t ret;
        ret = dma_queue_enqueue(&dma_handle, handle->config.dma_tag,
            _dma_upload_handler, handle);
        assert(ret == 0);
}

static void
_dma_upload_handler(const dma_queue_transfer_t *transfer)
{
        balls_handle_t handle = transfer->work;

        handle->load_count--;

        if (handle->load_count == 0) {
                if (handle->load_callback != NULL) {
                        handle->load_callback(handle);
                }
        }
}
