/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

#define STATE_ZOOM_MOVE_INVALID         (-1)
#define STATE_ZOOM_MOVE_ORIGIN          (0)
#define STATE_ZOOM_WAIT                 (1)
#define STATE_ZOOM_MOVE_ANCHOR          (2)
#define STATE_ZOOM_RELEASE_BUTTONS      (3)
#define STATE_ZOOM_SELECT_ANCHOR        (4)

#define ZOOM_TEX_PATH   "ZOOM.TEX"
#define ZOOM_PAL_PATH   "ZOOM.PAL"

#define ZOOM_POINT_WIDTH                (64)
#define ZOOM_POINT_HEIGHT               (102)
#define ZOOM_POINT_POINTER_SIZE         (3)
#define ZOOM_POINT_COLOR_SELECT         COLOR_RGB555( 0,  0, 31)
#define ZOOM_POINT_COLOR_WAIT           COLOR_RGB555(31,  0,  0)
#define ZOOM_POINT_COLOR_HIGHLIGHT      COLOR_RGB555( 0, 31,  0)

extern uint8_t root_romdisk[];

/* Zoom state */
static int32_t _state_zoom = STATE_ZOOM_MOVE_ORIGIN;
static int16_vector2_t _display = INT16_VECTOR2_INITIALIZER(ZOOM_POINT_WIDTH, ZOOM_POINT_HEIGHT);
static int16_vector2_t _zoom_point = INT16_VECTOR2_INITIALIZER(0, 0);
static uint16_t _zoom_point_value = CMDT_ZOOM_POINT_CENTER;
static uint32_t _delay_frames = 0;
static struct smpc_peripheral_digital _digital;

static struct {
        int16_vector2_t position;
        color_rgb555_t color;

        struct vdp1_cmdt_polygon polygon;
} _polygon_pointer;

static struct {
        fix16_t anim_rate;
        fix16_t anim_rate_dir;

        uint16_t *tex_base;
        uint16_t *pal_base;

        struct vdp1_cmdt_scaled_sprite sprite;
} _sprite;

static void *_romdisk = NULL;

static struct vdp1_cmdt_list *_env_cmdt_list = NULL;
static struct vdp1_cmdt_list *_cmdt_list = NULL;

static volatile uint32_t _frt_count = 0;

static inline bool __always_inline
_digital_dirs_pressed(void)
{
        return (_digital.pressed.raw & PERIPHERAL_DIGITAL_DIRECTIONS) != 0x0000;
}

static void _hardware_init(void);
static void _init(void);

static void _env_cmdt_list_config(void);

static void _state_zoom_move_origin(void);
static void _state_zoom_wait(void);
static void _state_zoom_move_anchor(void);
static void _state_zoom_release_buttons(void);
static void _state_zoom_select_anchor(void);

static void _sprite_init(void);
static void _sprite_config(void);
static void _polygon_pointer_init(void);
static void _polygon_pointer_config(void);

static void _dma_upload(void *, void *, size_t, uint8_t);

static void _vblank_out_handler(void);
static void _frt_ovi_handler(void);

int
main(void)
{
        static void (*const _state_zoom_funcs[])(void) = {
                _state_zoom_move_origin,
                _state_zoom_wait,
                _state_zoom_move_anchor,
                _state_zoom_release_buttons,
                _state_zoom_select_anchor
        };

        _hardware_init();
        _init();

        uint32_t frt_frame_a;
        frt_frame_a = 0;

        uint32_t frt_frame_b;
        frt_frame_b = 0;

        while (true) {
                frt_frame_a = 0;

                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                _state_zoom_funcs[_state_zoom]();

                vdp1_sync_draw(_env_cmdt_list);

                vdp1_cmdt_list_reset(_cmdt_list);
                _sprite_config();
                _polygon_pointer_config();
                vdp1_cmdt_end(_cmdt_list);

                vdp1_sync_draw(_cmdt_list);
                vdp1_sync_draw_wait();

                dbgio_flush();

                vdp_sync(0);

                frt_frame_b = cpu_frt_count_get();

                char buf[128];

                uint32_t frt_delta;
                frt_delta = frt_frame_b - frt_frame_a;

                (void)sprintf(buf, "[H[2J"
                    "a,b: %lu,%lu = %lu\n", frt_frame_a, frt_frame_b, frt_delta);
                dbgio_buffer(buf);
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        vdp2_sprite_priority_set(0, 6);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();

        smpc_init();
        smpc_peripheral_init();

        vdp_sync_vblank_out_set(_vblank_out_handler);

        cpu_frt_init(FRT_CLOCK_DIV_32);
        cpu_frt_ovi_set(_frt_ovi_handler);
}

static void
_init(void)
{
        dbgio_dev_default_init(DBGIO_DEV_VDP2);
        dbgio_dev_set(DBGIO_DEV_VDP2);

        romdisk_init();

        _romdisk = romdisk_mount("/", root_romdisk);
        assert(_romdisk != NULL);

        _env_cmdt_list_config();

        _cmdt_list = vdp1_cmdt_list_alloc(3);

        _polygon_pointer_init();
        _sprite_init();
}

static void
_env_cmdt_list_config(void)
{
        const struct vdp1_cmdt_system_clip_coord system_clip = {
                .coord.x = SCREEN_WIDTH - 1,
                .coord.y = SCREEN_HEIGHT - 1
        };

        const struct vdp1_cmdt_local_coord polygon_local_coord = {
                .coord.x = 0,
                .coord.y = 0
        };

        const struct vdp1_cmdt_polygon polygon = {
                .draw_mode.raw = 0x0000,
                .draw_mode.pre_clipping_disable = true,
                .color.raw = 0x0000,
                .vertices = {
                        {               0, SCREEN_HEIGHT - 1},
                        {SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1},
                        {SCREEN_WIDTH - 1,                 0},
                        {               0,                 0}
                }
        };

        const struct vdp1_cmdt_local_coord local_coord = {
                .coord.x = SCREEN_WIDTH / 2,
                .coord.y = SCREEN_HEIGHT / 2
        };

        _env_cmdt_list = vdp1_cmdt_list_alloc(7);

        vdp1_cmdt_system_clip_coord_add(_env_cmdt_list, &system_clip);
        vdp1_cmdt_local_coord_add(_env_cmdt_list, &polygon_local_coord);
        vdp1_cmdt_polygon_add(_env_cmdt_list, &polygon);
        vdp1_cmdt_local_coord_add(_env_cmdt_list, &local_coord);
        vdp1_cmdt_end(_env_cmdt_list);
}

static void
_dma_upload(void *dst, void *src, size_t len, uint8_t tag)
{
        const struct scu_dma_level_cfg scu_dma_level_cfg = {
                .mode = SCU_DMA_MODE_DIRECT,
                .stride = SCU_DMA_STRIDE_2_BYTES,
                .update = SCU_DMA_UPDATE_NONE,
                .xfer.direct.len = len,
                .xfer.direct.dst = (uint32_t)dst,
                .xfer.direct.src = CPU_CACHE_THROUGH | (uint32_t)src
        };

        struct scu_dma_reg_buffer reg_buffer;
        scu_dma_config_buffer(&reg_buffer, &scu_dma_level_cfg);

        int8_t ret;
        ret = dma_queue_enqueue(&reg_buffer, tag, NULL, NULL);
        assert(ret == 0);
}

static void
_sprite_init(void)
{
        _sprite.anim_rate = F16(0.0f);
        _sprite.anim_rate_dir = F16(1.0f);

        _sprite.tex_base = vdp1_vram_texture_base_get();
        _sprite.pal_base = (void *)VDP2_CRAM_MODE_1_OFFSET(1, 0, 0x0000);

        _sprite.sprite.draw_mode.color_mode = 4;
        _sprite.sprite.draw_mode.trans_pixel_disable = true;
        _sprite.sprite.draw_mode.pre_clipping_disable = true;
        _sprite.sprite.draw_mode.end_code_disable = true;
        _sprite.sprite.zoom_point.enable = true;
        _sprite.sprite.sprite_type.type_0.dc = 0x0100;
        _sprite.sprite.width = ZOOM_POINT_WIDTH;
        _sprite.sprite.height = ZOOM_POINT_HEIGHT;

        void *fh[2];
        void *p;
        size_t len;

        fh[0] = romdisk_open(_romdisk, ZOOM_TEX_PATH);
        assert(fh[0] != NULL);
        p = romdisk_direct(fh[0]);
        len = romdisk_total(fh[0]);
        _dma_upload(_sprite.tex_base, p, len, DMA_QUEUE_TAG_IMMEDIATE);

        fh[1] = romdisk_open(_romdisk, ZOOM_PAL_PATH);
        assert(fh[1] != NULL);
        p = romdisk_direct(fh[1]);
        len = romdisk_total(fh[1]);
        _dma_upload(_sprite.pal_base, p, len, DMA_QUEUE_TAG_IMMEDIATE);

        dma_queue_flush(DMA_QUEUE_TAG_IMMEDIATE);
        dma_queue_flush_wait();

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);
}

static void
_sprite_config(void)
{
        uint32_t offset;
        offset = fix16_to_int(_sprite.anim_rate) * (ZOOM_POINT_WIDTH * ZOOM_POINT_HEIGHT);

        _sprite.sprite.char_base = (uint32_t)_sprite.tex_base + offset;
        _sprite.sprite.zoom_point.raw = _zoom_point_value;
        _sprite.sprite.zoom.point.x = _zoom_point.x;
        _sprite.sprite.zoom.point.y = _zoom_point.y;
        _sprite.sprite.zoom.display.x = _display.x;
        _sprite.sprite.zoom.display.y = _display.y;

        _sprite.anim_rate += fix16_mul(_sprite.anim_rate_dir, F16(0.25f));

        if (_sprite.anim_rate <= F16(0.0f)) {
                _sprite.anim_rate_dir = -_sprite.anim_rate_dir;
                _sprite.anim_rate = F16(1.0f);
        } else if (fix16_abs(_sprite.anim_rate) >= F16(13.0f)) {
                _sprite.anim_rate_dir = -_sprite.anim_rate_dir;
                _sprite.anim_rate = F16(12.0f);
        }

        vdp1_cmdt_scaled_sprite_add(_cmdt_list, &_sprite.sprite);
}

static void
_polygon_pointer_init(void)
{
        (void)memset(&_polygon_pointer.polygon, 0x00, sizeof(_polygon_pointer.polygon));

        _polygon_pointer.polygon.draw_mode.raw = 0x0000;
        _polygon_pointer.polygon.draw_mode.pre_clipping_disable = true;
        _polygon_pointer.polygon.draw_mode.cc_mode = 0;
        _polygon_pointer.polygon.draw_mode.mesh_enable = false;
}

static void
_polygon_pointer_config(void)
{
        _polygon_pointer.polygon.color = _polygon_pointer.color;
        _polygon_pointer.polygon.vertex.a.x = ZOOM_POINT_POINTER_SIZE + _polygon_pointer.position.x - 1;
        _polygon_pointer.polygon.vertex.a.y = -ZOOM_POINT_POINTER_SIZE + _polygon_pointer.position.y;
        _polygon_pointer.polygon.vertex.b.x = ZOOM_POINT_POINTER_SIZE + _polygon_pointer.position.x - 1;
        _polygon_pointer.polygon.vertex.b.y = ZOOM_POINT_POINTER_SIZE + _polygon_pointer.position.y - 1;
        _polygon_pointer.polygon.vertex.c.x = -ZOOM_POINT_POINTER_SIZE + _polygon_pointer.position.x;
        _polygon_pointer.polygon.vertex.c.y = ZOOM_POINT_POINTER_SIZE + _polygon_pointer.position.y - 1;
        _polygon_pointer.polygon.vertex.d.x = -ZOOM_POINT_POINTER_SIZE + _polygon_pointer.position.x;
        _polygon_pointer.polygon.vertex.d.y = -ZOOM_POINT_POINTER_SIZE + _polygon_pointer.position.y;
        _polygon_pointer.polygon.grad_base = 0x00000000;

        vdp1_cmdt_polygon_add(_cmdt_list, &_polygon_pointer.polygon);
}

static void
_state_zoom_move_origin(void)
{
        _polygon_pointer.position.x = 0;
        _polygon_pointer.position.y = 0;

        _display.x = ZOOM_POINT_WIDTH - 1;
        _display.y = ZOOM_POINT_HEIGHT - 1;

        _zoom_point_value = CMDT_ZOOM_POINT_CENTER;
        _zoom_point.x = 0;
        _zoom_point.y = 0;

        _polygon_pointer.color = ZOOM_POINT_COLOR_SELECT;

        _delay_frames = 0;

        if (_digital_dirs_pressed()) {
                _state_zoom = STATE_ZOOM_WAIT;
        } else if ((_digital.held.button.a) != 0) {
                _state_zoom = STATE_ZOOM_RELEASE_BUTTONS;
        }
}

static void
_state_zoom_wait(void)
{
        _delay_frames++;

        if (_delay_frames > 9) {
                _delay_frames = 0;
                _state_zoom = STATE_ZOOM_MOVE_ANCHOR;
        } else if (!(_digital_dirs_pressed())) {
                _delay_frames = 0;
                _state_zoom = STATE_ZOOM_MOVE_ORIGIN;
        }
}

static void
_state_zoom_move_anchor(void)
{
        _polygon_pointer.position.x = 0;
        _polygon_pointer.position.y = 0;

        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_LEFT) != 0) {
                _polygon_pointer.position.x = -_display.x / 2;
        }

        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_RIGHT) != 0) {
                _polygon_pointer.position.x = _display.x / 2;
        }

        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_UP) != 0) {
                _polygon_pointer.position.y = -_display.y / 2;
        }

        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_DOWN) != 0) {
                _polygon_pointer.position.y = _display.y / 2;
        }

        /* Determine the zoom point */
        bool x_center;
        x_center = _polygon_pointer.position.x == 0;
        bool x_left;
        x_left = _polygon_pointer.position.x < 0;
        bool x_right;
        x_right = _polygon_pointer.position.x > 0;

        bool y_center;
        y_center = _polygon_pointer.position.y == 0;
        bool y_up;
        y_up = _polygon_pointer.position.y < 0;
        bool y_down;
        y_down = _polygon_pointer.position.y > 0;

        if (x_center && y_up) {
                _zoom_point_value = CMDT_ZOOM_POINT_UPPER_CENTER;
                _zoom_point.x = 0;
                _zoom_point.y = -ZOOM_POINT_HEIGHT / 2;
        } else if (x_center && y_down) {
                _zoom_point_value = CMDT_ZOOM_POINT_LOWER_CENTER;
                _zoom_point.x = 0;
                _zoom_point.y = ZOOM_POINT_HEIGHT / 2;
        } else if (y_center && x_left) {
                _zoom_point_value = CMDT_ZOOM_POINT_CENTER_LEFT;
                _zoom_point.x = -ZOOM_POINT_WIDTH / 2;
                _zoom_point.y = 0;
        } else if (y_center && x_right) {
                _zoom_point_value = CMDT_ZOOM_POINT_CENTER_RIGHT;
                _zoom_point.x = ZOOM_POINT_WIDTH / 2;
                _zoom_point.y = 0;
        } else if (y_up && x_left) {
                _zoom_point_value = CMDT_ZOOM_POINT_UPPER_LEFT;
                _zoom_point.x = -ZOOM_POINT_WIDTH / 2;
                _zoom_point.y = -ZOOM_POINT_HEIGHT / 2;
        } else if (y_up && x_right) {
                _zoom_point_value = CMDT_ZOOM_POINT_UPPER_RIGHT;
                _zoom_point.x = ZOOM_POINT_WIDTH / 2;
                _zoom_point.y = -ZOOM_POINT_HEIGHT / 2;
        } else if (y_down && x_left) {
                _zoom_point_value = CMDT_ZOOM_POINT_LOWER_LEFT;
                _zoom_point.x = -ZOOM_POINT_WIDTH / 2;
                _zoom_point.y = ZOOM_POINT_HEIGHT / 2;
        } else if (y_down && x_right) {
                _zoom_point_value = CMDT_ZOOM_POINT_LOWER_RIGHT;
                _zoom_point.x = ZOOM_POINT_WIDTH / 2;
                _zoom_point.y = ZOOM_POINT_HEIGHT / 2;
        } else if (x_center && y_center) {
                _zoom_point_value = CMDT_ZOOM_POINT_CENTER;
                _zoom_point.x = 0;
                _zoom_point.y = 0;
        }

        if (!(_digital_dirs_pressed())) {
                _state_zoom = STATE_ZOOM_MOVE_ORIGIN;
        } else if ((_digital.held.button.a) != 0) {
                _state_zoom = STATE_ZOOM_RELEASE_BUTTONS;
        }
}

static void
_state_zoom_release_buttons(void)
{
        _polygon_pointer.color = ZOOM_POINT_COLOR_WAIT;

        if (!(_digital_dirs_pressed())) {
                _state_zoom = STATE_ZOOM_SELECT_ANCHOR;
        }
}

static void
_state_zoom_select_anchor(void)
{
        static const struct zoom_point_boundary {
                int16_t w_dir; /* Display width direction */
                int16_t h_dir; /* Display height direction */
                int16_t x_max;
                int16_t y_max;
                int16_t x_min;
                int16_t y_min;
        } _zoom_point_boundaries[] = {
                { /* Upper left */
                        .w_dir = 1,
                        .h_dir = -1,
                        .x_min = 0,
                        .y_min = 0,
                        .x_max = (SCREEN_WIDTH / 2) + (ZOOM_POINT_WIDTH / 2),
                        .y_max = ZOOM_POINT_HEIGHT + ((SCREEN_HEIGHT - ZOOM_POINT_HEIGHT) / 2)
                }, { /* Upper center */
                        .w_dir = 1,
                        .h_dir = -1,
                        .x_min = 0,
                        .y_min = 0,
                        .x_max = SCREEN_WIDTH,
                        .y_max = ZOOM_POINT_HEIGHT + ((SCREEN_HEIGHT - ZOOM_POINT_HEIGHT) / 2)
                }, { /* Upper right */
                        .w_dir = -1,
                        .h_dir = -1,
                        .x_min = 0,
                        .y_min = 0,
                        .x_max = (SCREEN_WIDTH / 2) + (ZOOM_POINT_WIDTH / 2),
                        .y_max = ZOOM_POINT_HEIGHT + ((SCREEN_HEIGHT - ZOOM_POINT_HEIGHT) / 2)
                }, { /* Reserved */
                        0
                }, { /* Center left */
                        .w_dir = 1,
                        .h_dir = 1,
                        .x_min = 0,
                        .y_min = 0,
                        .x_max = (SCREEN_WIDTH / 2) + (ZOOM_POINT_WIDTH / 2),
                        .y_max = SCREEN_HEIGHT
                }, { /* Center */
                        .w_dir = 1,
                        .h_dir = -1,
                        .x_min = 0,
                        .y_min = 0,
                        .x_max = SCREEN_WIDTH,
                        .y_max = SCREEN_HEIGHT
                }, { /* Center right */
                        .w_dir = -1,
                        .h_dir = -1,
                        .x_min = 0,
                        .y_min = 0,
                        .x_max = (SCREEN_WIDTH / 2) + (ZOOM_POINT_WIDTH / 2),
                        .y_max = SCREEN_HEIGHT
                }, { /* Reserved */
                        0
                }, { /* Lower left */
                        .w_dir = 1,
                        .h_dir = 1,
                        .x_min = 0,
                        .y_min = 0,
                        .x_max = (SCREEN_WIDTH / 2) + (ZOOM_POINT_WIDTH / 2),
                        .y_max = ZOOM_POINT_HEIGHT + ((SCREEN_HEIGHT - ZOOM_POINT_HEIGHT) / 2)
                }, { /* Lower center */
                        .w_dir = 1,
                        .h_dir = 1,
                        .x_min = 0,
                        .y_min = 0,
                        .x_max = SCREEN_WIDTH,
                        .y_max = ZOOM_POINT_HEIGHT + ((SCREEN_HEIGHT - ZOOM_POINT_HEIGHT) / 2)
                }, { /* Lower right */
                        .w_dir = -1,
                        .h_dir = 1,
                        .x_min = 0,
                        .y_min = 0,
                        .x_max = (SCREEN_WIDTH / 2) + (ZOOM_POINT_WIDTH / 2),
                        .y_max = ZOOM_POINT_HEIGHT + ((SCREEN_HEIGHT - ZOOM_POINT_HEIGHT) / 2)
                }
        };

        _polygon_pointer.color = ZOOM_POINT_COLOR_HIGHLIGHT;

        uint32_t zp_idx;
        zp_idx = dlog2(_zoom_point_value) - 5;

        const struct zoom_point_boundary *zp_boundary;
        zp_boundary = &_zoom_point_boundaries[zp_idx];

        int16_t dw_dir;
        dw_dir = zp_boundary->w_dir * 1;

        int16_t dh_dir;
        dh_dir = zp_boundary->h_dir * 1;

        if ((_digital.pressed.button.up) != 0) {
                if (((_display.y + dh_dir) >= zp_boundary->y_min) &&
                    ((_display.y + dh_dir) <= zp_boundary->y_max)) {
                        _display.y = _display.y + dh_dir;
                }
        }

        if ((_digital.pressed.button.down) != 0) {
                if (((_display.y - dh_dir) >= zp_boundary->y_min) &&
                    ((_display.y - dh_dir) <= zp_boundary->y_max)) {
                        _display.y = _display.y - dh_dir;
                }
        }

        if ((_digital.pressed.button.left) != 0) {
                if (((_display.x - dw_dir) >= zp_boundary->x_min) &&
                    ((_display.x - dw_dir) <= zp_boundary->x_max)) {
                        _display.x = _display.x - dw_dir;
                }
        }

        if ((_digital.pressed.button.right) != 0) {
                if (((_display.x + dw_dir) >= zp_boundary->x_min) &&
                    ((_display.x + dw_dir) <= zp_boundary->x_max)) {
                        _display.x = _display.x + dw_dir;
                }
        }

        if ((_digital.held.button.b) != 0) {
                _state_zoom = STATE_ZOOM_MOVE_ORIGIN;
        }
}

static void
_vblank_out_handler(void)
{
        smpc_peripheral_intback_issue();
}

static void
_frt_ovi_handler(void)
{
        assert(false);
}
