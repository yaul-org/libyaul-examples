/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

extern uint8_t root_romdisk[];

static void _hardware_init(void);
static void _test_dsp_program(uint32_t);

static void _vblank_in_handler(irq_mux_handle_t *);

static uint32_t _ram0[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram1[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram2[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram3[DSP_RAM_PAGE_WORD_COUNT];

static void *_romdisk;

static char _buffer[256];

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        _romdisk = romdisk_mount("/", root_romdisk);

        scu_dsp_init();

        memset(_ram0, 0xAA, DSP_RAM_PAGE_SIZE);
        memset(_ram1, 0xBB, DSP_RAM_PAGE_SIZE);
        memset(_ram2, 0xCC, DSP_RAM_PAGE_SIZE);
        memset(_ram3, 0xDD, DSP_RAM_PAGE_SIZE);

        void *fh;
        fh = romdisk_open(_romdisk, "program.count");
        assert(fh != NULL);
        uint32_t program_count;
        romdisk_read(fh, &program_count, sizeof(program_count));
        romdisk_close(fh);

        (void)sprintf(_buffer, "Running %lu DSP programs\n", program_count);
        cons_buffer(_buffer);

        uint32_t program_id;
        for (program_id = 1; program_id <= program_count; program_id++) {
                _test_dsp_program(program_id);
        }

        cons_buffer("Passed");

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

static void
_test_dsp_program(uint32_t program_id __unused)
{
        uint32_t *program = (uint32_t *)0x20201000;

        char program_name[64] __unused;
        (void)sprintf(program_name, "%04lu.dsp.bin", program_id);

        MEMORY_WRITE(32, 0x00200000, program_id);

        void *fh;
        fh = romdisk_open(_romdisk, program_name);
        assert(fh != NULL);

        char mnemonic[48];
        romdisk_read(fh, mnemonic, sizeof(mnemonic));

        romdisk_read(fh, program, 8);
        romdisk_close(fh);

        scu_dsp_program_clear();
        scu_dsp_program_load(&program[0], 2);

        scu_dsp_data_write(0, 0, _ram0, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(1, 0, _ram1, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(2, 0, _ram2, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(3, 0, _ram3, DSP_RAM_PAGE_WORD_COUNT);

        scu_dsp_program_pc_set(0);

        (void)sprintf(_buffer, "[2;1H[0J"
            "program ID: %lu\n"
            "%s\n",
            program_id,
            mnemonic);
        cons_buffer(_buffer);
        vdp2_tvmd_vblank_out_wait();
        vdp2_tvmd_vblank_in_wait();
        cons_flush();

        scu_dsp_program_start();
        scu_dsp_program_end_wait();

        struct dsp_status status;
        memset(&status, 0, sizeof(struct dsp_status));

        scu_dsp_status_get(&status);

        if (status.pc > 0) {
                return;
        }

        sprintf(_buffer,
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
        cons_buffer(_buffer);

        cons_flush();
        abort();
}
