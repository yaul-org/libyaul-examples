/*
 * Copyright (c) 2012-2025 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

/*
 * adpcm driver is Copyright (C) 2021 celeriyacon - https://github.com/celeriyacon
 *
 */

#include <yaul.h>

#include <assert.h>
#include <stdlib.h>
#include "adpcm.h"
#include "adp68k.h"

extern uint8_t asset_sound_driver[];
extern uint8_t asset_sound_driver_end[];
extern uint8_t asset_ouch_adp[];
extern uint8_t asset_ouch_adp_end[];
extern uint8_t asset_short_adp[];
extern uint8_t asset_short_adp_end[];

static void _vblank_out_handler(void *);

static smpc_peripheral_digital_t _digital;
static cdfs_filelist_t _filelist;

void
reset_screen(void)
{
        dbgio_printf("[H[2J");
        dbgio_printf("ADPCM sample 1/2/3 start : A/B/C\r\n");
        dbgio_printf("ADPCM sample 1/2/3 stop  : X/Y/Z\r\n");
        dbgio_printf("\r\n");
        dbgio_printf("Sample 1 : 44100 4.5-bit 24.8 kB/s\r\n");
        dbgio_printf("\r\n");
        dbgio_printf("Sample 2 : 44100 2.5-bit 13.8 kB/s\r\n");
        dbgio_printf("\r\n");
        dbgio_printf("Sample 3 : 44100 1.5-bit 8.3 kB/s\r\n");
        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();
}

void adcpm_play_sample (int channel, int id)
{
        volatile ADPCMChannelControl* adpcc = &adp68k_scblock->adpcm[channel];
        while(SCSP_SCIPD & 0x20);
        adpcc->id = id;
        adpcc->volume[0] = 0x4000;
        adpcc->volume[1] = 0x4000;
        adpcc->action = ADP68K_ACTION_PLAY;
        SCSP_SCIPD_LO = 0x20;
}

void adcpm_stop_channel (int channel)
{
        volatile ADPCMChannelControl* adpcc = &adp68k_scblock->adpcm[channel];
        while(SCSP_SCIPD & 0x20);
        adpcc->action = ADP68K_ACTION_STOP;
        SCSP_SCIPD_LO = 0x20;
}

void adpcm_filesystem_init()
{
    cdfs_filelist_entry_t * const filelist_entries = cdfs_entries_alloc(-1);
    assert(filelist_entries != NULL);
    cdfs_config_default_set();
    cdfs_filelist_init(&_filelist, filelist_entries, -1);
    cdfs_filelist_root_read(&_filelist);
}


int adpcm_load_file(char * filename, void * buf)
{
        int ret;
        //blocking version, don't use in multitasking environment
	cdfs_filelist_entry_t _file;
	_file.starting_fad = 0;
	for (uint32_t i=0;i<_filelist.entries_count;i++)
	{
		if (0 == memcmp(_filelist.entries[i].name,filename,strlen(filename)))
			_file = _filelist.entries[i];
	}

        if (_file.starting_fad == 0) {
                dbgio_printf("Failed to load file!\r\n");
                return 0; //not found
        }

        const uint32_t sector_count = (_file.size + (CDFS_SECTOR_SIZE - 1)) / CDFS_SECTOR_SIZE;

        if ((ret = cd_block_sectors_read(_file.starting_fad, buf, sector_count*CDFS_SECTOR_SIZE)) != 0) {
                dbgio_printf("Failed to read file!\r\n");
	        return 0;
        }
        return _file.size;
}

int
main(void)
{
        uint32_t sound_ram_offset = 0;
        int stream_flag = 0;
        int key_processing_filter = 0;

        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        //load driver binary to SMPC
        adpcm_load_driver(asset_sound_driver, asset_sound_driver_end - asset_sound_driver);

        adpcm_filesystem_init();

        //stop all channels
        while(SCSP_SCIPD & 0x20);

        for(unsigned i = 0; i < 8; i++)
                adp68k_scblock->adpcm[i].action = ADP68K_ACTION_STOP;

        SCSP_SCIPD_LO = 0x20;
        while(SCSP_SCIPD & 0x20);

        //start of sound ram is a uint32 table of sample offsets, allocating 16 sounds
        sound_ram_offset = 0x2000 + 16*sizeof(uint32_t);
        
        //load "ouch" sample from assets into position 0
        //using channel 1 to stream this sample
        SCSP16(0x2000+0) = sound_ram_offset>>16;
        SCSP16(0x2000+2) = sound_ram_offset;
        memcpy((void*)SCSPVP(sound_ram_offset),asset_ouch_adp,asset_ouch_adp_end-asset_ouch_adp);
        sound_ram_offset += (asset_ouch_adp_end-asset_ouch_adp);

        //load "short" sample from assets into position 1
        //using channel 2 to stream this sample
        SCSP16(0x2000+4) = sound_ram_offset>>16;
        SCSP16(0x2000+6) = sound_ram_offset;
        memcpy((void*)SCSPVP(sound_ram_offset),asset_short_adp,asset_short_adp_end-asset_short_adp);
        sound_ram_offset += (asset_short_adp_end-asset_short_adp);

        //load first part of "awakening of the octopus" sample from cd into position 2
        //using channel 0 to stream this sample, because somehow it has a monitor set to MOBUF
        SCSP16(0x2000+8) = sound_ram_offset>>16;
        SCSP16(0x2000+10) = sound_ram_offset;
        int stream_file_size = adpcm_load_file("YAUL_P1.ADP",HWRAM(0x80000));
        memcpy((void*)SCSPVP(sound_ram_offset),HWRAM(0x80000),stream_file_size);
        //keeping sound_ram_offset at this location for streaming

        int stream_unit_size = *((uint16_t*)HWRAM(0x80000));//fitching the unit size from the file
        reset_screen();

        //states : 
        //=1 = load first file
        // 0 = load next file
        // 1 = waiting for half of the buffer
        // 2 = waiting for end of the buffer
        // 3 = last buffer
        int state = -1; 
        int next_file_num;
        char buf[16];

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                if (stream_flag) {
                        switch (state){
                                case -1:
                                        //loading first part
                                        stream_file_size = adpcm_load_file("YAUL_P1.ADP",HWRAM(0x80000));
                                        memcpy((void*)SCSPVP(sound_ram_offset),HWRAM(0x80000),stream_file_size);
                                        next_file_num=2;
                                        state = 0;
                                        adcpm_play_sample(0,2);
                                        break;
                                case 0:
                                        //loading next part
                                        sprintf(buf,"YAUL_P%i.ADP",next_file_num);
                                        adpcm_load_file(buf,HWRAM(0x80000));
                                        next_file_num++;
                                        state = 1;
                                        if (next_file_num == 23)
                                                state = 3;
                                        break;
                                case 1:
                                        //waiting for half of the buffer
                                        if (SCSP_CREG(0x8) > (stream_unit_size/0x20+0x40)) {
                                                //not copying header (5 bytes)
                                                for(int i = 5; i < stream_unit_size/2; i++)
                                                        SCSP8(sound_ram_offset+i) = *((uint8_t*)HWRAM(0x80000+i));
                                                for(int i = 5; i < stream_unit_size; i++)
                                                        SCSP8(sound_ram_offset+stream_unit_size+i) = *((uint8_t*)HWRAM(0x80000+stream_unit_size+i));
                                                state = 2;
                                        }
                                        break;
                                case 2:
                                        //waiting for buffer overwrap
                                        if (SCSP_CREG(0x8) < stream_unit_size/0x20) {
                                                for(int i = stream_unit_size/2; i < stream_unit_size; i++)
                                                        SCSP8(sound_ram_offset+i) = *((uint8_t*)HWRAM(0x80000+i));
                                                for(int i = stream_unit_size; i < stream_unit_size*2; i++)
                                                        SCSP8(sound_ram_offset+stream_unit_size+i) = *((uint8_t*)HWRAM(0x80000+stream_unit_size+i));
                                                state = 0;
                                        }
                                        break;
                                case 3: 
                                        //waiting for half of the buffer
                                        while (SCSP_CREG(0x8) <= (stream_unit_size/0x20+0x40)) 
                                                ;
                                        int last_stream_unit_size =  *((uint16_t*)HWRAM(0x80000));
                                        //last buffer
                                        for(int i = 5; i < last_stream_unit_size; i++)
                                                SCSP8(sound_ram_offset+i) = *((uint8_t*)HWRAM(0x80000+i));
                                        for(int i = 5; i < last_stream_unit_size*2; i++)
                                                SCSP8(sound_ram_offset+stream_unit_size+i) = *((uint8_t*)HWRAM(0x80000+last_stream_unit_size+i));

                                        //cleaning the buffer remainder
                                        for(int i = last_stream_unit_size; i < stream_unit_size/2; i++) {
                                                SCSP8(sound_ram_offset+i) = 0;
                                                SCSP8(sound_ram_offset+stream_unit_size+i*2) = 0;
                                                SCSP8(sound_ram_offset+stream_unit_size+i*2+1) = 0;
                                        }
                                        
                                        //waiting for buffer overwrap
                                        while (SCSP_CREG(0x8) >= stream_unit_size/0x20)
                                                ;

                                        //cleaning the buffer remainder, part2
                                        for(int i = stream_unit_size/2; i < stream_unit_size; i++) {
                                                SCSP8(sound_ram_offset+i) = 0;
                                                SCSP8(sound_ram_offset+stream_unit_size+i*2) = 0;
                                                SCSP8(sound_ram_offset+stream_unit_size+i*2+1) = 0;
                                        }

                                        //waiting for half of the last buffer
                                        while (SCSP_CREG(0x8) <= (stream_unit_size/0x20+0x40)) 
                                                ;

                                        //stop
                                        stream_flag = 0;
                                        state = -1;
                                        adcpm_stop_channel(0);
                                        break;
                        }
                }

                if (0 == key_processing_filter) {
                        if (_digital.pressed.button.a != 0) {
                                adcpm_play_sample(1,0);
                                key_processing_filter = 5;
                        }
                        else if (_digital.pressed.button.b != 0) {
                                adcpm_play_sample(2,1);
                                key_processing_filter = 5;
                        }
                        else if (_digital.pressed.button.c != 0) {
                                state = -1;
                                stream_flag = 1;
                                key_processing_filter = 5;
                        }
                        else if (_digital.pressed.button.x != 0) {
                                adcpm_stop_channel(1);
                                key_processing_filter = 5;
                        }
                        else if (_digital.pressed.button.y != 0) {
                                adcpm_stop_channel(2);
                                key_processing_filter = 5;
                        }
                        else if (_digital.pressed.button.z != 0) {
                                stream_flag = 0;
                                state = -1;
                                adcpm_stop_channel(0);
                                key_processing_filter = 5;
                        }
                }
                else{
                        key_processing_filter--;
                }

                //waiting for 1 frame
                vdp2_sync();
                vdp2_sync_wait();
        }
}

void
user_init(void)
{
        smpc_peripheral_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp2_tvmd_display_set();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}