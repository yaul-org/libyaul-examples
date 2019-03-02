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

#define STATE_ZOOM_MOVE_INVALID         -1
#define STATE_ZOOM_MOVE_ORIGIN          0
#define STATE_ZOOM_WAIT                 1
#define STATE_ZOOM_MOVE_ANCHOR          2
#define STATE_ZOOM_RELEASE_BUTTONS      3
#define STATE_ZOOM_SELECT_ANCHOR        4

#define ZOOM_POINT_WIDTH                160
#define ZOOM_POINT_HEIGHT               96
#define ZOOM_POINT_POINTER_SIZE         3
#define ZOOM_POINT_COLOR_SELECT         COLOR_RGB555( 0,  0, 31)
#define ZOOM_POINT_COLOR_WAIT           COLOR_RGB555(31,  0,  0)
#define ZOOM_POINT_COLOR_HIGHLIGHT      COLOR_RGB555( 0, 31,  0)

extern uint8_t root_romdisk[];

static int32_t _state_zoom = STATE_ZOOM_MOVE_ORIGIN;
static int16_vector2_t _pointer = INT16_VECTOR2_INITIALIZER(0, 0);
static int16_vector2_t _display = INT16_VECTOR2_INITIALIZER(ZOOM_POINT_WIDTH, ZOOM_POINT_HEIGHT);
static int16_vector2_t _zoom_point = INT16_VECTOR2_INITIALIZER(0, 0);
static uint16_t _zoom_point_value = CMDT_ZOOM_POINT_CENTER;
static color_rgb555_t _zoom_point_color = COLOR_RGB555(0, 0, 0);
static uint32_t _delay_frames = 0;
static struct smpc_peripheral_digital _digital;

static struct zoom_point_boundary {
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
                .x_max = SCREEN_WIDTH - (ZOOM_POINT_WIDTH / 2),
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
                .x_max = SCREEN_WIDTH - (ZOOM_POINT_WIDTH / 2),
                .y_max = ZOOM_POINT_HEIGHT + ((SCREEN_HEIGHT - ZOOM_POINT_HEIGHT) / 2)
        }, { /* Reserved */
                0
        }, { /* Center left */
                .w_dir = 1,
                .h_dir = 1,
                .x_min = 0,
                .y_min = 0,
                .x_max = SCREEN_WIDTH - (ZOOM_POINT_WIDTH / 2),
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
                .x_max = SCREEN_WIDTH - (ZOOM_POINT_WIDTH / 2),
                .y_max = SCREEN_HEIGHT
        }, { /* Reserved */
                0
        }, { /* Lower left */
                .w_dir = 1,
                .h_dir = 1,
                .x_min = 0,
                .y_min = 0,
                .x_max = SCREEN_WIDTH - (ZOOM_POINT_WIDTH / 2),
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
                .x_max = SCREEN_WIDTH - (ZOOM_POINT_WIDTH / 2),
                .y_max = ZOOM_POINT_HEIGHT + ((SCREEN_HEIGHT - ZOOM_POINT_HEIGHT) / 2)
        }
};

static void _hardware_init(void);

static void _setup_drawing_env(struct vdp1_cmdt_list *, bool);

static void _dma_immediate_upload(void *, void *, size_t);
static void _dma_upload(void *, void *, size_t, uint8_t);

static void _vblank_out_handler(void);

int
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2);
        dbgio_dev_set(DBGIO_DEV_VDP2);

        romdisk_init();

        void *romdisk;
        romdisk = romdisk_mount("/", root_romdisk);
        assert(romdisk != NULL);

        uint16_t *vdp1_tex;
        vdp1_tex = vdp1_vram_texture_base_get();

        uint16_t *vdp1_pal;
        vdp1_pal = (void *)VDP2_CRAM_MODE_1_OFFSET(1, 0, 0x0000);

        void *fh[2];
        void *p;
        size_t len;

        fh[0] = romdisk_open(romdisk, "ZOOM.TEX");
        assert(fh[0] != NULL);
        p = romdisk_direct(fh[0]);
        len = romdisk_total(fh[0]);
        _dma_immediate_upload(vdp1_tex, p, len);

        fh[1] = romdisk_open(romdisk, "ZOOM.PAL");
        assert(fh[1] != NULL);
        p = romdisk_direct(fh[1]);
        len = romdisk_total(fh[1]);
        _dma_immediate_upload(vdp1_pal, p, len);

        struct vdp1_cmdt_list *cmdt_list_env;
        cmdt_list_env = vdp1_cmdt_list_alloc(7);

        _setup_drawing_env(cmdt_list_env, true);

        vdp1_sync_draw(cmdt_list_env);

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);

        struct vdp1_cmdt_list *cmdt_list;
        cmdt_list = vdp1_cmdt_list_alloc(3);

        struct vdp1_cmdt_scaled_sprite sprite;
        (void)memset(&sprite, 0x00, sizeof(sprite));

        sprite.cs_mode.color_mode = 4;
        sprite.cs_mode.trans_pixel_disable = true;
        sprite.cs_mode.pre_clipping_disable = true;
        sprite.cs_mode.end_code_disable = true;
        sprite.cs_zoom_point.enable = true;
        sprite.cs_char = (uint32_t)vdp1_tex;
        sprite.cs_sprite_type.type_0.dc = 0x0100;
        sprite.cs_width = ZOOM_POINT_WIDTH;
        sprite.cs_height = ZOOM_POINT_HEIGHT;

        struct vdp1_cmdt_polygon polygon_pointer;
        (void)memset(&polygon_pointer, 0x00, sizeof(struct vdp1_cmdt_polygon));

        char *buffer;
        buffer = malloc(256);
        assert(buffer != NULL);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                dbgio_buffer("[H[2J");

                vdp1_cmdt_list_reset(cmdt_list);

                bool dirs_pressed;
                dirs_pressed = (_digital.pressed.raw & PERIPHERAL_DIGITAL_DIRECTIONS) != 0;

                bool x_center;
                bool x_left;
                bool x_right;
                bool y_center;
                bool y_up;
                bool y_down;

                struct zoom_point_boundary *zp_boundary;
                uint32_t zp_idx;

                int16_t dw_dir;
                int16_t dh_dir;

                switch (_state_zoom) {
                case STATE_ZOOM_MOVE_ORIGIN:
                        _pointer.x = 0;
                        _pointer.y = 0;

                        _display.x = ZOOM_POINT_WIDTH;
                        _display.y = ZOOM_POINT_HEIGHT;

                        _zoom_point_value = CMDT_ZOOM_POINT_CENTER;
                        _zoom_point.x = 0;
                        _zoom_point.y = 0;
                        _zoom_point_color = ZOOM_POINT_COLOR_SELECT;

                        _delay_frames = 0;

                        if (dirs_pressed) {
                                _state_zoom = STATE_ZOOM_WAIT;
                        } else if ((_digital.held.button.a) != 0) {
                                _state_zoom = STATE_ZOOM_RELEASE_BUTTONS;
                        }
                        break;
                case STATE_ZOOM_WAIT:
                        _delay_frames++;

                        if (_delay_frames > 9) {
                                _delay_frames = 0;
                                _state_zoom = STATE_ZOOM_MOVE_ANCHOR;
                        } else if (!dirs_pressed) {
                                _delay_frames = 0;
                                _state_zoom = STATE_ZOOM_MOVE_ORIGIN;
                        }
                        break;
                case STATE_ZOOM_MOVE_ANCHOR:
                        _pointer.x = 0;
                        _pointer.y = 0;

                        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_LEFT) != 0) {
                                _pointer.x = -_display.x / 2;
                        }

                        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_RIGHT) != 0) {
                                _pointer.x = _display.x / 2;
                        }

                        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_UP) != 0) {
                                _pointer.y = -_display.y / 2;
                        }

                        if ((_digital.pressed.raw & PERIPHERAL_DIGITAL_DOWN) != 0) {
                                _pointer.y = _display.y / 2;
                        }

                        /* Determine the zoom point */
                        x_center = _pointer.x == 0;
                        x_left = _pointer.x < 0;
                        x_right = _pointer.x > 0;

                        y_center = _pointer.y == 0;
                        y_up = _pointer.y < 0;
                        y_down = _pointer.y > 0;

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

                        if (!dirs_pressed) {
                                _state_zoom = STATE_ZOOM_MOVE_ORIGIN;
                        } else if ((_digital.held.button.a) != 0) {
                                _state_zoom = STATE_ZOOM_RELEASE_BUTTONS;
                        }
                        break;
                case STATE_ZOOM_RELEASE_BUTTONS:
                        _zoom_point_color = ZOOM_POINT_COLOR_WAIT;

                        if (!dirs_pressed) {
                                _state_zoom = STATE_ZOOM_SELECT_ANCHOR;
                        }
                        break;
                case STATE_ZOOM_SELECT_ANCHOR:
                        _zoom_point_color = ZOOM_POINT_COLOR_HIGHLIGHT;

                        zp_idx = dlog2(_zoom_point_value) - 5;
                        zp_boundary = &_zoom_point_boundaries[zp_idx];

                        dw_dir = zp_boundary->w_dir * 1;
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
                        break;
                }

                sprite.cs_zoom_point.raw = _zoom_point_value;
                sprite.cs_zoom.point.x = _zoom_point.x;
                sprite.cs_zoom.point.y = _zoom_point.y;
                sprite.cs_zoom.display.x = _display.x;
                sprite.cs_zoom.display.y = _display.y;

                polygon_pointer.cp_color = _zoom_point_color;
                polygon_pointer.cp_vertex.a.x = ZOOM_POINT_POINTER_SIZE + _pointer.x - 1;
                polygon_pointer.cp_vertex.a.y = -ZOOM_POINT_POINTER_SIZE + _pointer.y;
                polygon_pointer.cp_vertex.b.x = ZOOM_POINT_POINTER_SIZE + _pointer.x - 1;
                polygon_pointer.cp_vertex.b.y = ZOOM_POINT_POINTER_SIZE + _pointer.y - 1;
                polygon_pointer.cp_vertex.c.x = -ZOOM_POINT_POINTER_SIZE + _pointer.x;
                polygon_pointer.cp_vertex.c.y = ZOOM_POINT_POINTER_SIZE + _pointer.y - 1;
                polygon_pointer.cp_vertex.d.x = -ZOOM_POINT_POINTER_SIZE + _pointer.x;
                polygon_pointer.cp_vertex.d.y = -ZOOM_POINT_POINTER_SIZE + _pointer.y;

                vdp1_cmdt_scaled_sprite_add(cmdt_list, &sprite);
                vdp1_cmdt_polygon_add(cmdt_list, &polygon_pointer);
                vdp1_cmdt_end(cmdt_list);

                dbgio_flush();

                vdp1_sync_draw(cmdt_list_env);
                vdp1_sync_draw(cmdt_list);
                vdp_sync(0);
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
}

static void
_setup_drawing_env(struct vdp1_cmdt_list *cmdt_list, bool end)
{
        struct vdp1_cmdt_local_coord local_coord;

        struct vdp1_cmdt_system_clip_coord system_clip = {
                .scc_coord = {
                        .x = SCREEN_WIDTH - 1,
                        .y = SCREEN_HEIGHT - 1
                }
        };

        struct vdp1_cmdt_user_clip_coord user_clip = {
                .ucc_coords = {
                        {
                                .x = 0,
                                .y = 0
                        },
                        {
                                .x = SCREEN_WIDTH - 1,
                                .y = SCREEN_HEIGHT - 1
                        }
                }
        };

        vdp1_cmdt_system_clip_coord_add(cmdt_list, &system_clip);
        vdp1_cmdt_user_clip_coord_add(cmdt_list, &user_clip);

        local_coord.lc_coord.x = 0;
        local_coord.lc_coord.y = 0;

        vdp1_cmdt_local_coord_add(cmdt_list, &local_coord);

        struct vdp1_cmdt_polygon polygon;

        polygon.cp_mode.raw = 0x0000;
        polygon.cp_color.raw = 0x0000;

        polygon.cp_vertex.a.x = 0;
        polygon.cp_vertex.a.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.b.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.b.y = SCREEN_HEIGHT - 1;

        polygon.cp_vertex.c.x = SCREEN_WIDTH - 1;
        polygon.cp_vertex.c.y = 0;

        polygon.cp_vertex.d.x = 0;
        polygon.cp_vertex.d.y = 0;

        vdp1_cmdt_polygon_add(cmdt_list, &polygon);

        local_coord.lc_coord.x = SCREEN_WIDTH / 2;
        local_coord.lc_coord.y = SCREEN_HEIGHT / 2;

        vdp1_cmdt_local_coord_add(cmdt_list, &local_coord);

        if (end) {
                vdp1_cmdt_end(cmdt_list);
        }
}

static void
_dma_immediate_upload(void *dst, void *src, size_t len)
{
        _dma_upload(dst, src, len, DMA_QUEUE_TAG_IMMEDIATE);
}

static void
_dma_upload(void *dst, void *src, size_t len, uint8_t tag)
{
        struct scu_dma_level_cfg scu_dma_level_cfg;
        struct scu_dma_reg_buffer reg_buffer;

        scu_dma_level_cfg.dlc_mode = SCU_DMA_MODE_DIRECT;
        scu_dma_level_cfg.dlc_stride = SCU_DMA_STRIDE_2_BYTES;
        scu_dma_level_cfg.dlc_update = SCU_DMA_UPDATE_NONE;
        scu_dma_level_cfg.dlc_xfer.direct.len = len;
        scu_dma_level_cfg.dlc_xfer.direct.dst = (uint32_t)dst;
        scu_dma_level_cfg.dlc_xfer.direct.src = CPU_CACHE_THROUGH | (uint32_t)src;

        scu_dma_config_buffer(&reg_buffer, &scu_dma_level_cfg);

        int8_t ret;
        ret = dma_queue_enqueue(&reg_buffer, tag, NULL, NULL);
        assert(ret == 0);
}

static void
_vblank_out_handler(void)
{
        smpc_peripheral_intback_issue();
}
