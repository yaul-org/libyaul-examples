#include <yaul.h>

#include <stdlib.h>

#include "vdp1-balls.h"

#include "balls.h"

/* #pragma GCC push_options */
/* #pragma GCC optimize ("align-functions=16") */

extern uint8_t asset_ball_tex[];
extern uint8_t asset_ball_tex_end[];
extern uint8_t asset_ball_pal[];
extern uint8_t asset_ball_pal_end[];

struct balls_handle {
        balls_config_t config;
};

static vdp1_vram_t _sprite_tex_base;
static vdp2_cram_t _sprite_pal_base;

balls_handle_t *
balls_init(const balls_config_t config)
{
        struct balls_handle * const handle =
            malloc(sizeof(struct balls_handle));
        assert(handle != NULL);

        handle->config = config;

        for (uint32_t i = 0; i < handle->config.count; i++) {
                const int16_t rand_x = rand();
                const int16_t rand_y = rand();

                /* Bitwise AND 511 gives a maximum integer value of 31. Scale
                 * the value the ((SCREEN_WIDTH / 2) / 31).
                 *
                 * Bit 0 is mapped as the direction */
                const int16_t neg_x = ((rand_x & 2) == 0) ? 1 : -1;
                const int16_t pos_x = (rand_x & 511) * (neg_x * 5);
                const int16_t dir_x = rand_x & 1;

                const int16_t neg_y = ((rand_y & 2) == 0) ? 1 : -1;
                const int16_t pos_y = (rand_y & 511) * (neg_y * 3);
                const int16_t dir_y = rand_y & 1;

                handle->config.balls->pos_x[i] = pos_x | dir_x;
                handle->config.balls->pos_y[i] = pos_y | dir_y;
        }

        return handle;
}

void
balls_assets_init(balls_handle_t *handle __unused)
{
        vdp1_vram_partitions_t vdp1_vram_partitions;

        vdp1_vram_partitions_get(&vdp1_vram_partitions);

        _sprite_tex_base = (vdp1_vram_t)vdp1_vram_partitions.texture_base;
        _sprite_pal_base = VDP2_CRAM_MODE_1_OFFSET(1, 0, 0x0000);
}

void
balls_assets_load(balls_handle_t *handle __unused)
{
        scu_dma_transfer(0, (void *)_sprite_tex_base, asset_ball_tex, asset_ball_tex_end - asset_ball_tex);
        scu_dma_transfer(0, (void *)_sprite_pal_base, asset_ball_pal, asset_ball_pal_end - asset_ball_pal);
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
                .color_mode           = 0,
                .trans_pixel_disable  = false,
                .pre_clipping_disable = true,
                .end_code_disable     = false
        };

        vdp1_cmdt_t *cmdt;
        cmdt = (vdp1_cmdt_t *)VDP1_CMD_TABLE(index, 0);

        for (uint32_t i = 0; i < count; i++) {
                vdp1_cmdt_normal_sprite_set(cmdt);

                vdp1_cmdt_draw_mode_set(cmdt, draw_mode);

                const uint32_t rand_index = rand() & 15;
                const uint16_t palette_offset =
                    (_sprite_pal_base + (rand_index << 5)) & (VDP2_CRAM_SIZE - 1);
                const uint16_t palette_number = palette_offset >> 1;

                const vdp1_cmdt_color_bank_t color_bank ={
                        .type_0.dc = palette_number & VDP2_SPRITE_TYPE_0_DC_MASK
                };

                vdp1_cmdt_color_mode0_set(cmdt, color_bank);
                vdp1_cmdt_char_size_set(cmdt, BALL_WIDTH, BALL_HEIGHT);
                vdp1_cmdt_char_base_set(cmdt, _sprite_tex_base);

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

/* #pragma GCC pop_options */
