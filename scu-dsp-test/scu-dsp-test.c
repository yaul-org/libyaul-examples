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

static uint32_t _ram0[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram1[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram2[DSP_RAM_PAGE_WORD_COUNT];
static uint32_t _ram3[DSP_RAM_PAGE_WORD_COUNT];

static void *_romdisk = NULL;

static char _buffer[256];

int
main(void)
{
        _hardware_init();

        dbgio_dev_default_init(DBGIO_DEV_VDP2);
        dbgio_dev_set(DBGIO_DEV_VDP2);

        _romdisk = romdisk_mount("/", root_romdisk);

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
        dbgio_buffer(_buffer);

        uint32_t program_id;
        for (program_id = 1; program_id <= program_count; program_id++) {
                _test_dsp_program(program_id);
        }

        dbgio_buffer("Passed");

        dbgio_flush();
        vdp_sync(0);

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
            COLOR_RGB555(0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_test_dsp_program(uint32_t program_id)
{
        uint32_t *program = (uint32_t *)0x20201000;

        char program_name[64] __unused;
        (void)sprintf(program_name, "%04lu.dsp.bin", program_id);

        void *fh;
        fh = romdisk_open(_romdisk, program_name);
        assert(fh != NULL);

        char mnemonic[64];
        romdisk_read(fh, mnemonic, sizeof(mnemonic));

        uint32_t start_pc;
        romdisk_read(fh, &start_pc, sizeof(start_pc));

        uint32_t end_pc;
        romdisk_read(fh, &end_pc, sizeof(end_pc));

        uint32_t program_size;
        romdisk_read(fh, &program_size, sizeof(program_size));

        romdisk_read(fh, program, program_size);
        romdisk_close(fh);

        /* For instructions that invokes DMA D0->DSP */
        /* For instruction that invokes DMA to PRG RAM */
        uint32_t endi = 0xF8000000;

        uint32_t i;
        for (i = 0; i < DSP_RAM_PAGE_WORD_COUNT; i++) {
                _ram0[i] = CPU_CACHE_THROUGH | (uint32_t)&endi >> 2;
                _ram1[i] = CPU_CACHE_THROUGH | (uint32_t)&endi >> 2;
                _ram2[i] = CPU_CACHE_THROUGH | (uint32_t)&endi >> 2;
                _ram3[i] = CPU_CACHE_THROUGH | (uint32_t)&endi >> 2;
        }

        scu_dsp_data_write(0, 0, _ram0, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(1, 0, _ram1, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(2, 0, _ram2, DSP_RAM_PAGE_WORD_COUNT);
        scu_dsp_data_write(3, 0, _ram3, DSP_RAM_PAGE_WORD_COUNT);

        scu_dsp_program_clear();
        scu_dsp_program_load(&program[0], program_size / 4);
        scu_dsp_program_pc_set(start_pc);

        (void)sprintf(_buffer,
            "[2;1H[0J"
            "program ID: %lu\n"
            "%s\n",
            program_id,
            mnemonic);
        dbgio_buffer(_buffer);

        dbgio_flush();
        vdp_sync(0);

        scu_dsp_program_start();
        scu_dsp_program_end_wait();

        struct dsp_status status;
        memset(&status, 0, sizeof(struct dsp_status));

        scu_dsp_status_get(&status);

        if (status.pc == end_pc) {
                return;
        }

        (void)sprintf(_buffer,
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
        dbgio_buffer(_buffer);

        dbgio_flush();
        vdp_sync(0);

        abort();
}
