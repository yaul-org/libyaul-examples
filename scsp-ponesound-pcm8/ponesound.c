/*
 * Copyright (c) 2012-2025 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

/*
 *
 * Ponesound driver access layer
 * Based on pcmsys.c/h from Ponesound sources
 *
 */

#include "ponesound.h"
#include <smpc/map.h>
#include <smpc/smc.h>
#include <string.h>

static short adx_coef_tbl[8][2] = 
{
	{ADX_768_COEF_1,   ADX_768_COEF_2},
	{ADX_1152_COEF_1, ADX_1152_COEF_2},
	{ADX_1536_COEF_1, ADX_1536_COEF_2},
	{ADX_2304_COEF_1, ADX_2304_COEF_2},
	{ADX_640_COEF_1,   ADX_640_COEF_2},
	{ADX_960_COEF_1,   ADX_960_COEF_2},
	{ADX_1280_COEF_1, ADX_1280_COEF_2},
	{ADX_1920_COEF_1, ADX_1920_COEF_2}
};

sysComPara * m68k_com = (sysComPara *)(SCSP_RAM(0x408 + DRV_SYS_END));

void ponesound_set_master_volume(unsigned short volume)
{
	unsigned short * master_volume = (unsigned short *)(SCSP_RAM(0x100400));
	volume = (volume >= 0xF) ? 0xF : volume;
	master_volume[0] = 0x200 | (volume & 0xF);
}

short calculate_bytes_per_blank(int sampleRate, int is8Bit, int isPAL)
{
	int frameCount = (isPAL) ? 50 : 60;
	int sampleSize = (is8Bit) ? 8 : 16;
	return ((sampleRate * sampleSize)>>3)/frameCount;
}

short convert_bitrate_to_pitchword(short sampleRate)
{
	int octr;
	int shiftr;
	int fnsr;
	
	octr = PCM_CALC_OCT(sampleRate);
	shiftr = PCM_CALC_SHIFT_FREQ(octr);
	fnsr = PCM_CALC_FNS(sampleRate, shiftr);
	
	return PCM_SET_PITCH_WORD(octr, fnsr);
}

extern cdfs_filelist_t _filelist;

void ponesound_load_driver(uint8_t * driver_ptr, int driver_size, int master_adx_frequency)
{
	//Initial SMPC reset, somehow it is essential
    smpc_smc_call(SMPC_SMC_SNDOFF);
    smpc_smc_call(SMPC_SMC_SNDON);

	// Make sure SCSP is set to 512k mode
	*(unsigned char *)(SCSP(0x400)) = 0x02;

	/// Clear Sound Ram
	memset((void *)SCSP_RAM(0), 0, 0x80000);
	
	// Copy driver
	//The immediacy of these commands is important.
	//As per SEGA technical bulletin 51, the Sound CPU is not to be turned off for more than 0.5 seconds.
	// Turn off Sound CPU
	smpc_smc_call(SMPC_SMC_SNDOFF);
	smpc_smc_wait(0);
	// Set max master volume + 4mbit memory 
	ponesound_set_master_volume(0xF);
	//Copy the driver binary (code) over to sound RAM. The binary includes the vector table information.
	memcpy((void *)SCSP_RAM(0), driver_ptr, driver_size);//buffer, driver_file.size);
	//Set the ADX coefficients for the driver to use, if one was selected.
	m68k_com->drv_adx_coef_1 = adx_coef_tbl[master_adx_frequency][0];
	m68k_com->drv_adx_coef_2 = adx_coef_tbl[master_adx_frequency][1];
	// Turn on Sound CPU again
	smpc_smc_wait(0);
	smpc_smc_call(SMPC_SMC_SNDON);

	m68k_com->start = 0xFFFF;
}

void ponesound_load_8bit_pcm(void * pcm_ptr, int pcm_size, int pcmNumber, int loadOffset, int sampleRate)
{
	memcpy((void *)SCSP_RAM(loadOffset),pcm_ptr,pcm_size);

	m68k_com->pcmCtrl[pcmNumber].hiAddrBits = (unsigned short)( (unsigned int)loadOffset >> 16);
	m68k_com->pcmCtrl[pcmNumber].loAddrBits = (unsigned short)( (unsigned int)loadOffset & 0xFFFF);
	
	m68k_com->pcmCtrl[pcmNumber].pitchword = convert_bitrate_to_pitchword(sampleRate);
	m68k_com->pcmCtrl[pcmNumber].playsize = pcm_size;
	m68k_com->pcmCtrl[pcmNumber].bytes_per_blank = calculate_bytes_per_blank(sampleRate, 1, (VDP2_TVMD_TV_STANDARD_PAL == vdp2_tvmd_tv_standard_get()) ? 1 : 0); //Iniitalize as max volume
	m68k_com->pcmCtrl[pcmNumber].bitDepth = PCM_TYPE_8BIT; //Select 8-bit
	m68k_com->pcmCtrl[pcmNumber].loopType = 0; //Initialize as non-looping
	m68k_com->pcmCtrl[pcmNumber].volume = 7; //Iniitalize as max volume
}

void ponesound_pcm_play(short pcmNumber, char ctrlType, char volume)
{
	if(pcmNumber < 0) return;
	m68k_com->pcmCtrl[pcmNumber].sh2_permit = 1;
	m68k_com->pcmCtrl[pcmNumber].volume = volume;
	m68k_com->pcmCtrl[pcmNumber].loopType = ctrlType;
}

void ponesound_vblank_rq(void * work __unused)
{
	m68k_com->start = 1;	
}
