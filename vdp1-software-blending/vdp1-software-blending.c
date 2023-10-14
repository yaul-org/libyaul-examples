/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <gamemath/defs.h>

#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

#define VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX (0)
#define VDP1_CMDT_ORDER_USER_CLIP_INDEX          (1)
#define VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX       (2)
#define VDP1_CMDT_ORDER_CLEAR_POLYGON_INDEX      (3)
#define VDP1_CMDT_ORDER_SPRITE_INDEX             (4)
#define VDP1_CMDT_ORDER_DRAW_END_INDEX           (5)
#define VDP1_CMDT_ORDER_COUNT                    (VDP1_CMDT_ORDER_DRAW_END_INDEX + 1)

extern const int16_vec2_t flare_texture_dim;
extern const uint16_t flare_texture[];
extern const uint32_t flare_texture_size;

typedef struct {
        int16_vec2_t coords;
} render_state_t;

static vdp1_cmdt_list_t *_cmdt_list = NULL;
static vdp1_vram_partitions_t _vdp1_vram_partitions;

static smpc_peripheral_digital_t _digital;

static void _vblank_out_handler(void *work);
static void _sync_render_handler(void *work);

static void _cmdt_list_init(void);
static void _cmdt_list_populate(void);

static inline uint16_t *_fb_offset_calc(uint32_t x, uint32_t y) __always_inline;

static void _flare_blend(int16_t flare_x, int16_t flare_y);

int
main(void)
{
        render_state_t render_state;

        render_state.coords.x = 0;
        render_state.coords.y = 0;

        _cmdt_list_init();

        /* Copy flare texture to VDP1 */
        scu_dma_transfer(0, _vdp1_vram_partitions.texture_base, flare_texture, flare_texture_size);
        scu_dma_transfer_wait(0);

        vdp1_sync_render_set(_sync_render_handler, &render_state);

        uint32_t frame_count;
        frame_count = 0;

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_LEFT) != 0) {
                        render_state.coords.x -= 5;
                } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_RIGHT) != 0) {
                        render_state.coords.x += 5;
                }

                if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_UP) != 0) {
                        render_state.coords.y -= 5;
                } else if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_DOWN) != 0) {
                        render_state.coords.y += 5;
                }

                render_state.coords.x = clamp(render_state.coords.x, 0, 512 - flare_texture_dim.x);
                render_state.coords.y = clamp(render_state.coords.y, 0, 512 - flare_texture_dim.y);

                vdp1_sync_cmdt_list_put(_cmdt_list, 0);
                vdp1_sync_render();

                if (frame_count == 0) {
                        vdp2_sync();
                }

                vdp1_sync();
                vdp1_sync_wait();
                frame_count++;
        }

        return 0;
}

void
user_init(void)
{
        smpc_peripheral_init();

        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_MEDNAFEN_DEBUG);
        dbgio_dev_font_load();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 0, 0));

        vdp2_sprite_priority_set(0, 7);

        vdp1_env_default_set();
        vdp1_vram_partitions_get(&_vdp1_vram_partitions);

        /* Must be variable rate with no frame rate cap (<= -2). There seems to
         * be issues */
        vdp1_sync_interval_set(-2);

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp2_tvmd_display_set();
}

static void
_cmdt_list_init(void)
{
        _cmdt_list = vdp1_cmdt_list_alloc(VDP1_CMDT_ORDER_COUNT);
        assert(_cmdt_list != NULL);

        _cmdt_list_populate();
}

static void
_cmdt_list_sprite_init(void)
{
        vdp1_cmdt_t * const cmdt = &_cmdt_list->cmdts[VDP1_CMDT_ORDER_SPRITE_INDEX];

        const vdp1_cmdt_draw_mode_t draw_mode = {
                .color_mode = 5
        };

        vdp1_cmdt_normal_sprite_set(cmdt);
        vdp1_cmdt_draw_mode_set(cmdt, draw_mode);
        vdp1_cmdt_char_size_set(cmdt, flare_texture_dim.x, flare_texture_dim.y);
        vdp1_cmdt_char_base_set(cmdt, (uint32_t)_vdp1_vram_partitions.texture_base);

        cmdt->cmd_xa = (-flare_texture_dim.x / 2) - 1;
        cmdt->cmd_ya = (-flare_texture_dim.y / 2) - 1;
}

static void
_cmdt_list_clear_init(void)
{
        static const vdp1_cmdt_draw_mode_t draw_mode = {
                .pre_clipping_disable = true
        };

        static const int16_vec2_t points[] = {
                INT16_VEC2_INITIALIZER(-(SCREEN_WIDTH / 2),     -(SCREEN_HEIGHT / 2)),
                INT16_VEC2_INITIALIZER( (SCREEN_WIDTH / 2) - 1, -(SCREEN_HEIGHT / 2)),
                INT16_VEC2_INITIALIZER( (SCREEN_WIDTH / 2) - 1,  (SCREEN_HEIGHT / 2) - 1),
                INT16_VEC2_INITIALIZER(-(SCREEN_WIDTH / 2),      (SCREEN_HEIGHT / 2) - 1),
        };

        static const rgb1555_t color = RGB1555(0, 0, 0, 0);

        vdp1_cmdt_t * const cmdt =
            &_cmdt_list->cmdts[VDP1_CMDT_ORDER_CLEAR_POLYGON_INDEX];

        vdp1_cmdt_polygon_set(cmdt);
        vdp1_cmdt_draw_mode_set(cmdt, draw_mode);
        vdp1_cmdt_color_set(cmdt, color);
        vdp1_cmdt_vtx_set(cmdt, points);
}

static void
_cmdt_list_populate(void)
{
        vdp1_env_preamble_populate(_cmdt_list->cmdts, NULL);

        _cmdt_list_clear_init();
        _cmdt_list_sprite_init();

        vdp1_cmdt_end_set(&_cmdt_list->cmdts[VDP1_CMDT_ORDER_DRAW_END_INDEX]);

        _cmdt_list->count = VDP1_CMDT_ORDER_COUNT;
}

static inline uint16_t * __always_inline
_fb_offset_calc(uint32_t x, uint32_t y)
{
        return (uint16_t *)VDP1_FB(((y * 512) + x) * sizeof(rgb1555_t));
}

static void
_flare_blend(int16_t flare_x, int16_t flare_y)
{
        /* Draw grey box */
        for (int32_t y = 0; y < flare_texture_dim.y; y++) {
                volatile rgb1555_t *fb = (volatile rgb1555_t *)_fb_offset_calc(0, y);

                for (int32_t x = 0; x < flare_texture_dim.x; x++, fb++) {
                        fb->raw = 0xBDEF;
                }
        }

        for (int32_t y = 0; y < flare_texture_dim.y; y++) {
                volatile rgb1555_t *fb =
                    (volatile rgb1555_t *)_fb_offset_calc(flare_x, flare_y + y);

                const rgb1555_t *flare_texture_offset =
                    (const rgb1555_t *)&flare_texture[y * flare_texture_dim.x];

                for (int32_t x = 0; x < flare_texture_dim.x; x++, fb++, flare_texture_offset++) {
                        const rgb1555_t src_pixel = (rgb1555_t)*flare_texture_offset;

                        if (src_pixel.raw == 0x0000) {
                                continue;
                        }

                        const rgb1555_t fb_pixel = *fb;

                        if (!fb_pixel.msb) {
                                *fb = src_pixel;
                        } else {
                                const uint16_t fb_raw = fb_pixel.raw;
                                const uint16_t src_raw = src_pixel.raw;

                                const uint16_t fb_r = fb_raw & 31;
                                const uint16_t fb_g = fb_raw & (31 << 5);
                                const uint16_t fb_b = fb_raw & (31 << 10);

                                const uint16_t src_r = src_raw & 31;
                                const uint16_t src_g = src_raw & (31 << 5);
                                const uint16_t src_b = src_raw & (31 << 10);

                                const uint16_t r = min(src_r + fb_r, 31);
                                const uint16_t g = min(src_g + fb_g, 31 << 5);
                                const uint16_t b = min(src_b + fb_b, 31 << 10);

                                fb->raw = (0x8000 | b | g | r);
                        }
                }
        }
}

static void
_sync_render_handler(void *work)
{
        render_state_t * const render_state = work;

        _flare_blend(render_state->coords.x, render_state->coords.y);
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}
