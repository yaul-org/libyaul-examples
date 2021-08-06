/*
 * Copyright (c) 2012-2021 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 * Nikita Sokolov <hitomi2500@mail.ru>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>

//some test locations for read and write buffers
#define SCI_BUFFER 0x260F0000
#define SCI_BUFFER_RECV 0x26080000

//doing 64K SCI transaction
#define TEST_SEQUENCE_LENGTH 0x10000 

static void _dmac_handler0(void *);

static volatile uint16_t _frt = 0;
static volatile uint32_t _ovf = 0;
static volatile bool _done = false;

//there is no core func for this yet, should be moved to not-yet-existent SCI driver 
static inline void __always_inline
cpu_sci_interrupt_priority_set(uint8_t priority)
{
        MEMORY_WRITE_AND(16, CPU(IPRB), 0x7FFF);
        MEMORY_WRITE_OR(16, CPU(IPRB), (priority & 0x0F) << 12);
}

void
sci_setup()
{
	MEMORY_WRITE(8,CPU(SCR),0x00); //stop all
	MEMORY_WRITE(8,CPU(SMR),0x80); //sync mode, 8bit, no parity, no MP, 1/4 clock
	MEMORY_WRITE(8,CPU(BRR),0x00); //maximum baudrate
	MEMORY_WRITE(8,CPU(SCR),0x01); //internal clock output
	MEMORY_WRITE(8,CPU(SCR),0xF1); //interrupts on, RX/TX on, internal clock output
}

int
main(void)
{
        int i=0;

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        cpu_dmac_interrupt_priority_set(8);
        cpu_sci_interrupt_priority_set(0); //disable interrupts from SCI, because they will be served by DMAC, not CPU
 
        cpu_dmac_cfg_t cfg0 __unused = {
                .channel= 0,
                .src = SCI_BUFFER,
                .src_mode = CPU_DMAC_SOURCE_INCREMENT,
                .dst = CPU(TDR),
                .dst_mode = CPU_DMAC_DESTINATION_FIXED,
                .len = TEST_SEQUENCE_LENGTH, 
                .stride = CPU_DMAC_STRIDE_1_BYTE,
                .request_mode = CPU_DMAC_REQUEST_MODE_MODULE,
                .detect_mode = CPU_DMAC_DETECT_MODE_EDGE,
                .bus_mode = CPU_DMAC_BUS_MODE_CYCLE_STEAL,
                .resource_select = CPU_DMAC_RESOURCE_SELECT_TXI,
                .nondefault = true,
                .ihr = _dmac_handler0,
                .ihr_work = NULL
        };

        cpu_dmac_cfg_t cfg1 __unused = {
                .channel= 1,
                .src = CPU(RDR),
                .src_mode = CPU_DMAC_SOURCE_FIXED,
                .dst = SCI_BUFFER_RECV,
                .dst_mode = CPU_DMAC_DESTINATION_INCREMENT,
                .len = TEST_SEQUENCE_LENGTH,
                .stride = CPU_DMAC_STRIDE_1_BYTE,
                .request_mode = CPU_DMAC_REQUEST_MODE_MODULE,
                .detect_mode = CPU_DMAC_DETECT_MODE_EDGE,
                .bus_mode = CPU_DMAC_BUS_MODE_CYCLE_STEAL,
                .resource_select = CPU_DMAC_RESOURCE_SELECT_RXI,
                .nondefault = true,
                .ihr = NULL,
                .ihr_work = NULL
        };
                
        sci_setup();

        //prepare write buffer
        for (i=0;i<TEST_SEQUENCE_LENGTH;i++)
        {
                MEMORY_WRITE(8,SCI_BUFFER+i,0xA5+i);
        }

        //clear read buffer
        memset((uint8_t*)SCI_BUFFER_RECV,0x00,TEST_SEQUENCE_LENGTH);

        //apply config to DMA channels
        cpu_dmac_channel_config_set(&cfg0);
        cpu_dmac_channel_config_set(&cfg1);

        //set dmac priority to round-robin
        cpu_dmac_priority_mode_set(CPU_DMAC_PRIORITY_MODE_ROUND_ROBIN);
        //enable dmac, because priority mode set disables it
        cpu_dmac_enable();
        
        //fire DMA
        _done = false;
        MEMORY_WRITE(8,CPU(SSR),0x00); //reset all SCI status flags
        MEMORY_WRITE(8,CPU(SCR),0x00); //stop SCI
        cpu_dmac_channel_start(1); //start the read channel first, so it's with sync with write channel
        cpu_dmac_channel_start(0); //start the write channel
        MEMORY_WRITE(8,CPU(SCR),0xF1); //enable SCI back, the requests to DMAC shoukld start automatically
                
        //wait for DMA
        while (!_done) ;

        //datacheck
        int iErrors = 0;
        uint8_t written,readen;
        for (i=0;i<TEST_SEQUENCE_LENGTH;i++)
        {
                readen = MEMORY_READ(8,SCI_BUFFER_RECV+i);
                written = 0xA5+i;

                if (readen !=written)
                {
                        if (iErrors < 10)
                        {
                                dbgio_printf("ERR: pos %u wr = 0x%02lX rd = 0x%02lX\n",iErrors,written,readen);
                                dbgio_flush();
                                vdp_sync();
                        }
                        iErrors ++; 
                }
        }

        dbgio_printf("SCI loopback test error count = %u\n",iErrors);

        if (iErrors)
        {
                dbgio_printf("\nSCI loopback test failed.\nEither pins 5 and 6 on CN6 (serial port)\nare not shorted, or you're running \nthe test within an emulator.\n");
        }
        else
        {
                dbgio_printf("\nSCI loopback test was sucessful.\n");
        }

        dbgio_flush();
        vdp_sync(); 

        //stop
        while (1);

        return 0;
}

void
user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_screen_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            COLOR_RGB1555(1, 0, 3, 15));

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_dmac_handler0(void *work __unused)
{
        _done = true;
}
