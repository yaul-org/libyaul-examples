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

#define STATE_UV_MOVE_INVALID         (-1)
#define STATE_UV_MOVE_ORIGIN          (0)
#define STATE_UV_WAIT                 (1)
#define STATE_UV_MOVE_ANCHOR          (2)
#define STATE_UV_RELEASE_BUTTONS      (3)
#define STATE_UV_SELECT_ANCHOR        (4)

#define UV_TEX_PATH   "ZOOM.TEX"
#define UV_PAL_PATH   "ZOOM.PAL"

#define UV_POINT_WIDTH                (32)
#define UV_POINT_HEIGHT               (32)
#define UV_POINT_POINTER_SIZE         (3)
#define UV_POINT_COLOR_SELECT         COLOR_RGB1555(1,  0,  0, 31)
#define UV_POINT_COLOR_WAIT           COLOR_RGB1555(1, 31,  0,  0)
#define UV_POINT_COLOR_HIGHLIGHT      COLOR_RGB1555(1,  0, 31,  0)

#define ORDER_SYSTEM_CLIP_COORDS_INDEX      0
#define ORDER_USER_CLIP_COORDS_INDEX        1
#define ORDER_CLEAR_LOCAL_COORDS_INDEX      2
#define ORDER_CLEAR_POLYGON_INDEX           3
#define ORDER_LOCAL_COORDS_INDEX            4
#define ORDER_SPRITE_INDEX                  5
#define ORDER_POLYGON_POINTER_INDEX         6
#define ORDER_DRAW_END_INDEX                7
#define ORDER_COUNT                         8

extern uint8_t root_romdisk[];

/* State */
static int32_t _state_uv        = STATE_UV_MOVE_ORIGIN;
static int16_vec2_t _uv_coords[4];
static uint16_t _uv_index       = 0;
static uint32_t _delay_frames   = 0;

static smpc_peripheral_digital_t _digital;

static struct {
        int16_vec2_t position;
        color_rgb1555_t color;

        vdp1_cmdt_t *cmdt;
} _polygon_pointer;

static struct {
        vdp1_cmdt_t *cmdt;
} _sprite;

static void *_romdisk = NULL;

static vdp1_cmdt_list_t *_cmdt_list = NULL;
static vdp1_vram_partitions_t _vdp1_vram_partitions;

static volatile uint32_t _frt_count = 0;

static inline bool __always_inline
_digital_dirs_pressed(void)
{
        return (_digital.pressed.raw & PERIPHERAL_DIGITAL_DIRECTIONS) != 0x0000;
}

static void _init(void);

static void _cmdt_list_init(void);

static void _state_uv_move_origin(void);
static void _state_uv_wait(void);
static void _state_uv_move_anchor(void);
static void _state_uv_release_buttons(void);
static void _state_uv_select_anchor(void);

static void _sprite_init(void);
static void _sprite_config(void);
static void _polygon_pointer_init(void);
static void _polygon_pointer_config(void);

static uint32_t _frame_time_calculate(void);

static void _vblank_out_handler(void *);
static void _cpu_frt_ovi_handler(void);

int
main(void)
{
        static void (*const _state_uv_funcs[])(void) = {
                _state_uv_move_origin,
                _state_uv_wait,
                _state_uv_move_anchor,
                _state_uv_release_buttons,
                _state_uv_select_anchor
        };

        _init();

        _sprite_config();
        _polygon_pointer_config();

        cpu_frt_count_set(0);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                _state_uv_funcs[_state_uv]();

                _sprite_config();
                _polygon_pointer_config();

                vdp1_sync_cmdt_list_put(_cmdt_list, 0, NULL, NULL);

                dbgio_printf("[H[2J");

                for (uint32_t i = 0; i < 4; i++) {
                        dbgio_printf("%i. (%2i,%2i)\n", i, _uv_coords[i].x, _uv_coords[i].y);
                }

                dbgio_flush();

                vdp_sync();

                uint32_t result;
                result = _frame_time_calculate();

                char fixed[16];
                fix16_str(result, fixed, 7);

                dbgio_printf("\n%sms\n", fixed);
        }

        return 0;
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        vdp2_sprite_priority_set(0, 7);
        vdp2_sprite_priority_set(1, 7);
        vdp2_sprite_priority_set(2, 7);
        vdp2_sprite_priority_set(3, 7);
        vdp2_sprite_priority_set(4, 7);
        vdp2_sprite_priority_set(5, 7);
        vdp2_sprite_priority_set(6, 7);
        vdp2_sprite_priority_set(7, 7);

        /* Setup default VDP1 environment */
        vdp1_env_default_set();

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();

        vdp_sync_vblank_out_set(_vblank_out_handler);

        cpu_frt_init(CPU_FRT_CLOCK_DIV_32);
        cpu_frt_ovi_set(_cpu_frt_ovi_handler);

        vdp1_vram_partitions_get(&_vdp1_vram_partitions);
}

static void
_init(void)
{
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        romdisk_init();

        _romdisk = romdisk_mount("/", root_romdisk);
        assert(_romdisk != NULL);

        _cmdt_list_init();

        _uv_coords[0].x = 0;
        _uv_coords[0].y = 0;

        _uv_coords[1].x = 31;
        _uv_coords[1].y = 0;

        _uv_coords[2].x = 31;
        _uv_coords[2].y = 31;

        _uv_coords[3].x = 0;
        _uv_coords[3].y = 31;
}

static void
_cmdt_list_init(void)
{
        static const int16_vec2_t system_clip_coord =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                   SCREEN_HEIGHT - 1);

        static const int16_vec2_t user_clip_ul =
            INT16_VEC2_INITIALIZER(0,
                                   0);

        static const int16_vec2_t user_clip_lr =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,
                                   SCREEN_HEIGHT - 1);

        static const int16_vec2_t local_coord_ul =
            INT16_VEC2_INITIALIZER(0,
                                   0);

        static const int16_vec2_t local_coord_center =
            INT16_VEC2_INITIALIZER(SCREEN_WIDTH / 2,
                                   SCREEN_HEIGHT / 2);

        static const vdp1_cmdt_draw_mode_t polygon_draw_mode = {
                .raw = 0x0000,
                .bits.pre_clipping_disable = true
        };

        static const int16_vec2_t polygon_points[] = {
                INT16_VEC2_INITIALIZER(0, SCREEN_HEIGHT - 1),
                INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1),
                INT16_VEC2_INITIALIZER(SCREEN_WIDTH - 1,                 0),
                INT16_VEC2_INITIALIZER(               0,                 0)
        };

        _cmdt_list = vdp1_cmdt_list_alloc(ORDER_COUNT);

        (void)memset(&_cmdt_list->cmdts[0], 0x00, sizeof(vdp1_cmdt_t) * ORDER_COUNT);

        _cmdt_list->count = ORDER_COUNT;

        vdp1_cmdt_t *cmdts;
        cmdts = &_cmdt_list->cmdts[0];

        _sprite.cmdt = &cmdts[ORDER_SPRITE_INDEX];
        _polygon_pointer.cmdt = &cmdts[ORDER_POLYGON_POINTER_INDEX];

        _polygon_pointer_init();
        _sprite_init();

        vdp1_cmdt_system_clip_coord_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_SYSTEM_CLIP_COORDS_INDEX], CMDT_VTX_SYSTEM_CLIP, &system_clip_coord);

        vdp1_cmdt_user_clip_coord_set(&cmdts[ORDER_USER_CLIP_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_USER_CLIP_COORDS_INDEX], CMDT_VTX_USER_CLIP_UL, &user_clip_ul);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_USER_CLIP_COORDS_INDEX], CMDT_VTX_USER_CLIP_LR, &user_clip_lr);

        vdp1_cmdt_local_coord_set(&cmdts[ORDER_CLEAR_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_LOCAL_COORDS_INDEX], CMDT_VTX_LOCAL_COORD, &local_coord_ul);

        vdp1_cmdt_polygon_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX]);
        vdp1_cmdt_param_draw_mode_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], polygon_draw_mode);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], CMDT_VTX_POLYGON_A, &polygon_points[0]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], CMDT_VTX_POLYGON_B, &polygon_points[1]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], CMDT_VTX_POLYGON_C, &polygon_points[2]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_CLEAR_POLYGON_INDEX], CMDT_VTX_POLYGON_D, &polygon_points[3]);
        vdp1_cmdt_jump_skip_next(&cmdts[ORDER_CLEAR_POLYGON_INDEX]);

        vdp1_cmdt_local_coord_set(&cmdts[ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_param_vertex_set(&cmdts[ORDER_LOCAL_COORDS_INDEX], CMDT_VTX_LOCAL_COORD, &local_coord_center);

        vdp1_cmdt_end_set(&cmdts[ORDER_DRAW_END_INDEX]);
}

static uint32_t
_frame_time_calculate(void)
{
        uint16_t frt;
        frt = cpu_frt_count_get();

        cpu_frt_count_set(0);

        uint32_t delta_fix;
        delta_fix = frt << 4;

        uint32_t divisor_fix;
        divisor_fix = CPU_FRT_NTSC_320_32_COUNT_1MS << 4;

        cpu_divu_32_32_set(delta_fix << 4, divisor_fix);

        uint32_t result;
        result = cpu_divu_quotient_get();
        result <<= 12;

        return result;
}

static void
_sprite_init(void)
{
        vdp1_gouraud_table_t * const gouraud_table_base =
            _vdp1_vram_partitions.gouraud_base;

        color_rgb1555_t * const cram_base __unused =
            (color_rgb1555_t *)VDP2_CRAM_MODE_0_OFFSET(0, 0, 0x0000);

        const vdp1_cmdt_draw_mode_t draw_mode = {
                .bits.hss_enable = true,
                .bits.user_clipping_mode = 2,
                .bits.cc_mode = 4
        };

        const vdp1_cmdt_color_bank_t color_bank = {
                .type_0.data.dc = 0x0000,
                .type_0.data.cc = 0x00,
                .type_0.data.pr = 0x00
        };

        MEMORY_WRITE(16, LWRAM(0), draw_mode.raw);

        (void)memset(_sprite.cmdt, 0, sizeof(vdp1_cmdt_t));

        vdp1_cmdt_polygon_set(_sprite.cmdt);
        vdp1_cmdt_param_draw_mode_set(_sprite.cmdt, draw_mode);
        vdp1_cmdt_param_color_mode0_set(_sprite.cmdt, color_bank);

        _sprite.cmdt->cmd_xa = -UV_POINT_WIDTH;
        _sprite.cmdt->cmd_ya = -UV_POINT_HEIGHT;
        _sprite.cmdt->cmd_xb =  UV_POINT_WIDTH;
        _sprite.cmdt->cmd_yb = -UV_POINT_HEIGHT;
        _sprite.cmdt->cmd_xc =  UV_POINT_WIDTH;
        _sprite.cmdt->cmd_yc =  UV_POINT_HEIGHT;
        _sprite.cmdt->cmd_xd = -UV_POINT_WIDTH;
        _sprite.cmdt->cmd_yd =  UV_POINT_HEIGHT;

        vdp1_cmdt_param_color_set(_sprite.cmdt, COLOR_RGB1555(0, 16, 16, 0));
        vdp1_cmdt_param_gouraud_base_set(_sprite.cmdt, (vdp1_vram_t)gouraud_table_base);

        void *fh[2];

        fh[0] = romdisk_open(_romdisk, UV_TEX_PATH);
        assert(fh[0] != NULL);
        const uint8_t * const tex_ptr __unused = romdisk_direct(fh[0]);

        fh[1] = romdisk_open(_romdisk, UV_PAL_PATH);
        assert(fh[1] != NULL);
        color_rgb1555_t * const pal_ptr __unused = romdisk_direct(fh[1]);

        for (uint32_t y = 0, cram_offset = 0; y < UV_POINT_HEIGHT; y++) {
                for (uint32_t x = 0; x < UV_POINT_WIDTH; x++) {
                        /* XXX: Hard coded values */
                        const uint32_t tex_offset = 16 + x + ((y + 48) * 64);

                        const uint32_t tex_index = tex_ptr[tex_offset];
                        const color_rgb1555_t color = pal_ptr[tex_index];

                        cram_base[cram_offset] = color;

                        cram_offset++;
                }
        }

        romdisk_close(fh[0]);
        romdisk_close(fh[1]);
}

static void
_sprite_config(void)
{
        vdp1_gouraud_table_t * const gouraud_table_base =
            _vdp1_vram_partitions.gouraud_base;

        for (uint32_t i = 0; i < 4; i++) {
                gouraud_table_base->colors[i] = COLOR_RGB1555(1, _uv_coords[i].x, _uv_coords[i].y, 0);
        }
}

static void
_polygon_pointer_init(void)
{
        static const vdp1_cmdt_draw_mode_t draw_mode = {
                .raw = 0x0000,
                .bits.pre_clipping_disable = true
        };

        vdp1_cmdt_polygon_set(_polygon_pointer.cmdt);
        vdp1_cmdt_param_draw_mode_set(_polygon_pointer.cmdt, draw_mode);
}

static void
_polygon_pointer_config(void)
{
        int16_vec2_t points[4];

        points[0].x = UV_POINT_POINTER_SIZE + _polygon_pointer.position.x - 1;
        points[0].y = -UV_POINT_POINTER_SIZE + _polygon_pointer.position.y;
        points[1].x = UV_POINT_POINTER_SIZE + _polygon_pointer.position.x - 1;
        points[1].y = UV_POINT_POINTER_SIZE + _polygon_pointer.position.y - 1;
        points[2].x = -UV_POINT_POINTER_SIZE + _polygon_pointer.position.x;
        points[2].y = UV_POINT_POINTER_SIZE + _polygon_pointer.position.y - 1;
        points[3].x = -UV_POINT_POINTER_SIZE + _polygon_pointer.position.x;
        points[3].y = -UV_POINT_POINTER_SIZE + _polygon_pointer.position.y;

        vdp1_cmdt_param_color_set(_polygon_pointer.cmdt, _polygon_pointer.color);

        vdp1_cmdt_param_vertex_set(_polygon_pointer.cmdt, CMDT_VTX_POLYGON_A, &points[0]);
        vdp1_cmdt_param_vertex_set(_polygon_pointer.cmdt, CMDT_VTX_POLYGON_B, &points[1]);
        vdp1_cmdt_param_vertex_set(_polygon_pointer.cmdt, CMDT_VTX_POLYGON_C, &points[2]);
        vdp1_cmdt_param_vertex_set(_polygon_pointer.cmdt, CMDT_VTX_POLYGON_D, &points[3]);
}

static void
_state_uv_move_origin(void)
{
        _polygon_pointer.position.x = 0;
        _polygon_pointer.position.y = 0;

        _uv_index = 0;

        _polygon_pointer.color = UV_POINT_COLOR_SELECT;

        _delay_frames = 0;

        if (_digital_dirs_pressed()) {
                _state_uv = STATE_UV_WAIT;
        }
}

static void
_state_uv_wait(void)
{
        _delay_frames++;

        /* XXX: Magic number */
        if (_delay_frames > 9) {
                _delay_frames = 0;
                _state_uv = STATE_UV_MOVE_ANCHOR;
        } else if (!(_digital_dirs_pressed())) {
                _delay_frames = 0;
                _state_uv = STATE_UV_MOVE_ORIGIN;
        }
}

static void
_state_uv_move_anchor(void)
{
        const bool x_left = ((_digital.pressed.raw & PERIPHERAL_DIGITAL_LEFT) != 0);
        const bool x_right = ((_digital.pressed.raw & PERIPHERAL_DIGITAL_RIGHT) != 0);
        const bool y_up = ((_digital.pressed.raw & PERIPHERAL_DIGITAL_UP) != 0);
        const bool y_down = ((_digital.pressed.raw & PERIPHERAL_DIGITAL_DOWN) != 0);

        bool selected;
        selected = true;

        if (y_up && x_left) {
                _uv_index = 0;
                _polygon_pointer.position.x = -(UV_POINT_WIDTH - 1);
                _polygon_pointer.position.y = -(UV_POINT_HEIGHT - 1);
        } else if (y_up && x_right) {
                _uv_index = 1;
                _polygon_pointer.position.x = UV_POINT_WIDTH;
                _polygon_pointer.position.y = -(UV_POINT_HEIGHT - 1);
        } else if (y_down && x_left) {
                _uv_index = 2;
                _polygon_pointer.position.x = -(UV_POINT_WIDTH - 1);
                _polygon_pointer.position.y = UV_POINT_HEIGHT;
        } else if (y_down && x_right) {
                _uv_index = 3;
                _polygon_pointer.position.x = UV_POINT_WIDTH;
                _polygon_pointer.position.y = UV_POINT_HEIGHT;
        } else {
                selected = false;
        }

        if (!selected) {
                _state_uv = STATE_UV_MOVE_ORIGIN;
        } else if ((_digital.held.button.a) != 0) {
                _state_uv = STATE_UV_RELEASE_BUTTONS;
        }
}

static void
_state_uv_release_buttons(void)
{
        _polygon_pointer.color = UV_POINT_COLOR_WAIT;

        if (!(_digital_dirs_pressed())) {
                _state_uv = STATE_UV_SELECT_ANCHOR;
        }
}

static void
_state_uv_select_anchor(void)
{
        _polygon_pointer.color = UV_POINT_COLOR_HIGHLIGHT;

        int16_vec2_t * const uv_coord = &_uv_coords[_uv_index];

        if ((_digital.pressed.button.up) != 0) {
                if (((uv_coord->y + 1) >= 0) && ((uv_coord->y + 1) <= 31)) {
                        uv_coord->y = uv_coord->y + 1;
                }
        }

        if ((_digital.pressed.button.down) != 0) {
                if (((uv_coord->y - 1) >= 0) && ((uv_coord->y - 1) <= 31)) {
                        uv_coord->y = uv_coord->y - 1;
                }
        }

        if ((_digital.pressed.button.left) != 0) {
                if (((uv_coord->x - 1) >= 0) && ((uv_coord->x - 1) <= 31)) {
                        uv_coord->x = uv_coord->x - 1;
                }
        }

        if ((_digital.pressed.button.right) != 0) {
                if (((uv_coord->x + 1) >= 0) && ((uv_coord->x + 1) <= 31)) {
                        uv_coord->x = uv_coord->x + 1;
                }
        }

        if ((_digital.held.button.b) != 0) {
                _state_uv = STATE_UV_MOVE_ORIGIN;
        }
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
