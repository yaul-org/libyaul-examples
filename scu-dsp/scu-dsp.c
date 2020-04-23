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

static void _dsp_end(void);

static uint32_t _ram0[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram1[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram2[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram3[DSP_RAM_PAGE_WORD_COUNT];

int
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);

        const uint32_t program[] = {
                /* See test1.dsp */
                0x00001C00,
                0x00021D00,
                0x02494000,
                0x01000000,
                0x18003209,
                0x00000000,
                0x00000000,
                0x00000000,
                0x00000000,
                0x00000000,
                0x00000000,
                0x00000000,
                0xF8000000
        };

        dbgio_buffer("Running DSP\n");

        memset(_ram0, 0xAA, DSP_RAM_PAGE_SIZE);
        memset(_ram1, 0xBB, DSP_RAM_PAGE_SIZE);
        memset(_ram2, 0xCC, DSP_RAM_PAGE_SIZE);
        memset(_ram3, 0xDD, DSP_RAM_PAGE_SIZE);

        _ram0[0] = 0x00000003;
        _ram1[0] = 0x00000002;

        scu_dsp_program_load(&program[0], sizeof(program) / sizeof(*program));

        scu_dsp_data_write(0, 0, _ram0, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(1, 0, _ram1, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(2, 0, _ram2, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(3, 0, _ram3, DSP_RAM_PAGE_WORD_COUNT);

        char buffer[128];

        do {
                uint8_t pc;
                pc = scu_dsp_program_step();

                sprintf(buffer, "PC: %02X\n", pc);
                dbgio_buffer(buffer);
                /* dbgio_flush() needs to be called during VBLANK-IN */
                dbgio_flush();
                vdp_sync();
        } while (!(scu_dsp_program_end()));

        _dsp_end();

        struct dsp_status status;
        scu_dsp_status_get(&status);

        sprintf(buffer,
            "\nT0 S Z C V E EX PC\n"
            " %X"
            " %X"
            " %X"
            " %X"
            " %X"
            " %X"
            "  %X"
            " %02X\n",
            status.t0,
            status.s,
            status.z,
            status.c,
            status.v,
            status.e,
            status.ex,
            status.pc);
        dbgio_buffer(buffer);

        dbgio_flush();
        vdp_sync();

        while (true) {
        }

        return 0;
}

static void
_hardware_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB555(1, 0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_dsp_end(void)
{
        char buffer[32];

        uint32_t a;
        uint32_t b;
        uint32_t c;

        scu_dsp_data_read(0, 0, &a, 1);
        scu_dsp_data_read(1, 0, &b, 1);
        scu_dsp_data_read(2, 0, &c, 1);

        sprintf(buffer, "Result: %lu * %lu = %lu\n", a, b, c);

        dbgio_buffer(buffer);
}
