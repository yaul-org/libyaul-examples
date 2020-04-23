/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>

static void _hardware_init(void);

static void _vblank_in_handler(void);
static void _vblank_out_handler(void);
static void _hblank_in_handler(void);

static void _timer0_handler(void);
static void _timer1_handler(void);

static color_rgb555_t _colors[] __unused = {
        COLOR_RGB555(1, 15,  3,  0),
        COLOR_RGB555(1,  0,  3, 15),
        COLOR_RGB555(1, 31,  3, 15),
        COLOR_RGB555(1, 15, 31,  0)
};

static uint16_t *_back_screen = (uint16_t *)VDP2_VRAM_ADDR(3, 0x01FFFE);
static uint16_t _line = 0;

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        cpu_intc_mask_set(0);

        scu_timer_init();

        scu_timer_t0_set(_timer0_handler);
        scu_timer_t1_set(_timer1_handler);

        char buffer[64];

        while (true) {
                vdp2_tvmd_vblank_out_wait();

                (void)sprintf(buffer, "[10;1H[2J%i\n", _line);
                cons_buffer(buffer);

                vdp2_tvmd_vblank_in_wait();
                cons_flush();
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_scrn_back_screen_color_set((uint32_t)_back_screen, COLOR_RGB555(1, 0, 3, 15));

        uint32_t scu_mask;
        scu_mask = IC_MASK_VBLANK_IN | IC_MASK_VBLANK_OUT | IC_MASK_HBLANK_IN;

        scu_ic_mask_chg(IC_MASK_ALL, scu_mask);

        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_IN, _vblank_in_handler);
        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_OUT, _vblank_out_handler);
        scu_ic_ihr_set(IC_INTERRUPT_HBLANK_IN, _hblank_in_handler);

        scu_ic_mask_chg(~scu_mask, IC_MASK_NONE);

        /* Enable interrupts */
        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_vblank_in_handler(void)
{
        vdp2_commit();
}

static void
_vblank_out_handler(void)
{
        *_back_screen = _colors[0].raw;

        scu_timer_t1_line_set(111);

        _line = 0;
}

static void
_hblank_in_handler(void)
{
        scu_timer_t1_value_set(1);
}

static void
_timer0_handler(void)
{
        scu_timer_t0_line_set(112);

        MEMORY_WRITE_AND(16, VDP2(EXTEN), ~0x0200);

        _line = MEMORY_READ(16, VDP2(VCNT));
}

static void
_timer1_handler(void)
{
        *_back_screen = _colors[112 - _line].raw;
}
