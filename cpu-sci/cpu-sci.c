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
#define SCI_BUFFER_SEND 0x26080000
#define SCI_BUFFER_RECV 0x26090000

#define TEST_PATTERN_LENGTH 0x10000 
#define TEST_PATTERN_START 0xA1 

static void _dmac_handler(void *);
static void __unused _sci_handler(void *);

static volatile bool _dma_done = false;
static volatile bool _sci_done = false;

int
main(void)
{
        int i=0;

        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();
        dbgio_dev_font_load_wait();

        //For a first part, let's setup SCI in a "tandem" mode, where it works together with DMAC

        cpu_dmac_interrupt_priority_set(8);
        cpu_sci_interrupt_priority_set(0); //disable interrupts from SCI, because they will be served by DMAC, not CPU
 
        cpu_dmac_cfg_t cfg0 __unused = {
                .channel= 0,
                .src = SCI_BUFFER_SEND,
                .src_mode = CPU_DMAC_SOURCE_INCREMENT,
                .dst = CPU(TDR),
                .dst_mode = CPU_DMAC_DESTINATION_FIXED,
                .len = TEST_PATTERN_LENGTH, 
                .stride = CPU_DMAC_STRIDE_1_BYTE,
                .request_mode = CPU_DMAC_REQUEST_MODE_MODULE,
                .detect_mode = CPU_DMAC_DETECT_MODE_EDGE,
                .bus_mode = CPU_DMAC_BUS_MODE_CYCLE_STEAL,
                .resource_select = CPU_DMAC_RESOURCE_SELECT_TXI,
                .nondefault = true,
                .ihr = _dmac_handler,
                .ihr_work = NULL
        };

        cpu_dmac_cfg_t cfg1 __unused = {
                .channel= 1,
                .src = CPU(RDR),
                .src_mode = CPU_DMAC_SOURCE_FIXED,
                .dst = SCI_BUFFER_RECV,
                .dst_mode = CPU_DMAC_DESTINATION_INCREMENT,
                .len = TEST_PATTERN_LENGTH,
                .stride = CPU_DMAC_STRIDE_1_BYTE,
                .request_mode = CPU_DMAC_REQUEST_MODE_MODULE,
                .detect_mode = CPU_DMAC_DETECT_MODE_EDGE,
                .bus_mode = CPU_DMAC_BUS_MODE_CYCLE_STEAL,
                .resource_select = CPU_DMAC_RESOURCE_SELECT_RXI,
                .nondefault = true,
                .ihr = NULL,
                .ihr_work = NULL
        };

        //interrupt handlers are never called, because SCI interrupts are masked for CPU
        //but we need to pass some non-null handlers to enable interrupts because DMAC needs them
        cpu_sci_cfg_t cfg_sci __unused = {
                .mode = CPU_SCI_MODE_SYNC,
                .ihr_rxi = _sci_handler,
                .ihr_txi = _sci_handler,
                .sck_config = CPU_SCI_SCK_OUTPUT
        };

      
        //sci_setup();
        cpu_sci_config_set(&cfg_sci);

        //prepare write buffer
        for (i=0;i<TEST_PATTERN_LENGTH;i++)
        {
                MEMORY_WRITE(8,SCI_BUFFER_SEND+i,TEST_PATTERN_START+i);
        }

        //clear read buffer
        memset((uint8_t*)SCI_BUFFER_RECV,0x00,TEST_PATTERN_LENGTH);

        //apply config to DMA channels
        cpu_dmac_channel_config_set(&cfg0);
        cpu_dmac_channel_config_set(&cfg1);

        //set dmac priority to round-robin
        cpu_dmac_priority_mode_set(CPU_DMAC_PRIORITY_MODE_ROUND_ROBIN);
        //enable dmac, because priority mode set disables it
        cpu_dmac_enable();
        
        //start SCI with DMAC, it requires disabling and enabling it in a dmac mode
        _dma_done = false;
        cpu_sci_reset_status();
        cpu_sci_disable();
        cpu_dmac_channel_start(1); //start the read channel first, so it's with sync with write channel
        cpu_dmac_channel_start(0); //start the write channel
        cpu_sci_enable_with_dmac(&cfg_sci); //enable SCI back, the requests to DMAC should start automatically
        
        //wait for DMA
        while (!_dma_done) ;

        //datacheck
        int iErrors = 0;
        uint8_t written,readen;
        for (i=0;i<TEST_PATTERN_LENGTH;i++)
        {
                readen = MEMORY_READ(8,SCI_BUFFER_RECV+i);
                written = TEST_PATTERN_START+i;

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

        dbgio_printf("SCI-DMAC loopback error count = %u\n",iErrors);

        if (iErrors)
        {
                dbgio_printf("\nSCI-DMAC loopback test failed.\nEither pins 5 and 6 on CN6 (serial port)\nare not shorted, or you're running \nthe test within an emulator.\n\n");
        }
        else
        {
                dbgio_printf("\nSCI-DMAC loopback test was sucessful.\n\n");
        }

        dbgio_flush();
        vdp_sync(); 

        //For a second part, let's setup SCI in a normal mode, with CPU interrupt and without DMAC link
        //Second par doesn't use SCI interrupts, because it crashes from unknown reason when enabling SCI interrupts

        cpu_dmac_disable();
        cpu_sci_disable();

        //disabling RXI and TXI interrupts.
        cfg_sci.ihr_rxi = NULL;
        cfg_sci.ihr_txi = NULL;

        //enabling TEI (end of transfer) interrupt
        cfg_sci.ihr_tei = _sci_handler;

        //re-setup the sci 
        cpu_sci_config_set(&cfg_sci);

        //reset any prior interrupts
        cpu_sci_reset_status();

        //enable interrupts from SCI
        //cpu_sci_interrupt_priority_set(8); // <= this is where it crashes

        //clear read buffer
        memset((uint8_t*)SCI_BUFFER_RECV,0x00,TEST_PATTERN_LENGTH);

        cpu_sci_enable();

        cpu_sci_get_read_value(); //do a dummy read to clear RDRF

        //do a bunch of transfers
        for (i=0;i<TEST_PATTERN_LENGTH;i++)
        {
                cpu_sci_set_write_value(TEST_PATTERN_START+i);
                cpu_sci_wait();
                MEMORY_WRITE(8,SCI_BUFFER_RECV+i,cpu_sci_get_read_value());
        }

        //check
        iErrors = 0;
        for (i=0;i<TEST_PATTERN_LENGTH;i++)
        {
                readen = MEMORY_READ(8,SCI_BUFFER_RECV+i);
                written = TEST_PATTERN_START+i;

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

        dbgio_printf("SCI loopback error count = %u\n",iErrors);

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


        while (true) {
        }

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
_dmac_handler(void *work __unused)
{
        _dma_done = true;
}

static void __unused
_sci_handler(void *work __unused)
{
        _sci_done = true;
}

