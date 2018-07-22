/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

static void _hardware_init(void);

static void _vblank_in_handler(irq_mux_handle_t *);

static uint32_t _ram0[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram1[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram2[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram3[DSP_RAM_PAGE_WORD_COUNT];

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        scu_dsp_init();

        const uint32_t program[] = {
                0x00000000,     /* NOP */
                0x00000000,     /* NOP */
                0x00000000,     /* NOP */
                0x00000000,     /* NOP */

                0x00000000,     /* NOP */
                0x00000000,     /* NOP */
                0x00000000,     /* NOP */
                0xFFFFFFFF      /* ENDI */
        };

        cons_buffer("Running DSP\n");

        memset(_ram0, 0xAA, DSP_RAM_PAGE_SIZE);
        memset(_ram1, 0xBB, DSP_RAM_PAGE_SIZE);
        memset(_ram2, 0xCC, DSP_RAM_PAGE_SIZE);
        memset(_ram3, 0xDD, DSP_RAM_PAGE_SIZE);

        scu_dsp_program_load(0, &program[0], 8);

        scu_dsp_data_write(0, 0, _ram0, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(1, 0, _ram1, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(2, 0, _ram2, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(3, 0, _ram3, DSP_RAM_PAGE_WORD_COUNT);

        scu_dsp_program_start();
        scu_dsp_program_end_wait();

        char buffer[32];
        sprintf(buffer, "PC: %i\n", scu_dsp_program_counter_get());

        cons_buffer(buffer);

        while (true) {
                vdp2_tvmd_vblank_out_wait();

                vdp2_tvmd_vblank_in_wait();
                cons_flush();
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_init();

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        irq_mux_t *vblank_in;
        vblank_in = vdp2_tvmd_vblank_in_irq_get();
        irq_mux_handle_add(vblank_in, _vblank_in_handler, NULL);

        /* Enable interrupts */
        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        vdp2_commit();
}
