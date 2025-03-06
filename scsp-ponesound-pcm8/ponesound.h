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

#include <stdint.h>
#include "yaul.h"

#define ADX_MASTER_768 (0)
#define ADX_MASTER_1152 (1)
#define ADX_MASTER_1536 (2)
#define ADX_MASTER_2304 (3)
/* 7.68 data */
#define ADX_768_COEF_1 (4401)
#define ADX_768_COEF_2 (-1183)
/* 11.52 data */
#define ADX_1152_COEF_1 (5386)
#define ADX_1152_COEF_2 (-1771)
/* 15.36 data */
#define ADX_1536_COEF_1 (5972)
#define ADX_1536_COEF_2 (-2187)
/* 23.04 data */
#define ADX_2304_COEF_1 (6631)
#define ADX_2304_COEF_2 (-2685)
// 6400 Hz for PAL
#define ADX_PAL_640 (4)
#define ADX_640_COEF_1 (3915)
#define ADX_640_COEF_2 (-936)
// 9600 Hz for PAL
#define ADX_PAL_960 (5)
#define ADX_960_COEF_1 (4963)
#define ADX_960_COEF_2 (-1504)
// 12800 Hz for PAL
#define ADX_PAL_1280 (6)
#define ADX_1280_COEF_1 (5612)
#define ADX_1280_COEF_2 (-1923)
// 19200 Hz for PAL
#define ADX_PAL_1920 (7)
#define ADX_1920_COEF_1 (6359)
#define ADX_1920_COEF_2 (-2469)

#define	PCM_ALT_LOOP	(3)
#define PCM_RVS_LOOP	(2)
#define PCM_FWD_LOOP	(1)
#define PCM_VOLATILE	(0)
#define PCM_PROTECTED	(-1)
#define PCM_SEMI		(-2)
#define ADX_STREAM		(-3)

#define PCM_TYPE_ADX (2) // 4-bit (compressed audio)
#define PCM_TYPE_8BIT (1) // 8-bit
#define PCM_TYPE_16BIT (0) // 16-bit

#define SCSP_RAM(x)                 (0x25A00000UL + (x))
// #define SNDPRG (SNDRAM(0x408))
#define DRV_SYS_END (47 * 1024) //System defined safe end of driver's address space

static const int logtbl[] = {
/* 0 */		0, 
/* 1 */		1, 
/* 2 */		2, 2, 
/* 4 */		3, 3, 3, 3, 
/* 8 */		4, 4, 4, 4, 4, 4, 4, 4, 
/* 16 */	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 
/* 32 */	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
			6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 
/* 64 */	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 
/* 128 */	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 
			8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	};

#define PCM_MSK1(a)				((a)&0x0001)
#define PCM_MSK3(a)				((a)&0x0007)
#define PCM_MSK4(a)				((a)&0x000F)
#define PCM_MSK5(a)				((a)&0x001F)
#define PCM_MSK10(a)			((a)&0x03FF)

#define PCM_SCSP_FREQUENCY					(44100L)

#define PCM_CALC_OCT(smpling_rate) 											\
		((int)logtbl[PCM_SCSP_FREQUENCY / ((smpling_rate) + 1)])
		
#define PCM_CALC_SHIFT_FREQ(oct)											\
		(PCM_SCSP_FREQUENCY >> (oct))
		
#define PCM_CALC_FNS(smpling_rate, shift_freq)								\
		((((smpling_rate) - (shift_freq)) << 10) / (shift_freq))
		
#define PCM_SET_PITCH_WORD(oct, fns)										\
		((int)((PCM_MSK4(-(oct)) << 11) | PCM_MSK10(fns)))

typedef struct {
	char loopType; //[0,1,2,3] No loop, normal loop, reverse loop, alternating loop
	unsigned char bitDepth; //0, 1, or 2; 0 is 16-bit, 1 is 8-bit, 2 is ADX
	unsigned short hiAddrBits; //bits 19-16 of...
	unsigned short loAddrBits; //Two 16-bit chunks that when combined, form the start address of the sound.
	unsigned short LSA; //The # of samples forward from the start address to return to after loop.
	unsigned short playsize; //The # of samples to play before the sound shall loop. **Otherwise used as the length of the sound.** Do not leave at 0!
						//8 bit PCM is 1 byte per sample. 16 bit PCM is 2 bytes per sample. Therefore an 8bit PCM is a maximum of 64KB, and 16bit is 128KB.
	unsigned short pitchword; //the OCT & FNS word to use in the ICSR, verbatim.
	unsigned char pan; //Direct pan setting
	unsigned char volume; //Direct volume setting
	unsigned short bytes_per_blank; //Bytes the PCM will play every time the driver is run (vblank)
	unsigned short decompression_size; //Size of the buffer used for an ADX sound effect. Specifically sized by Master SH2.
	unsigned char sh2_permit; //Does the SH2 permit this command? If TRUE, run the command. If FALSE, key its ICSR OFF.
	char icsr_target; //Which explicit ICSR is this to land in? Can be controlled by SH2 or by driver.
} _PCM_CTRL; //Driver Local Command Struct

typedef struct {
	volatile unsigned int adx_stream_length; //Length of the ADX stream (in ADX frames)
	volatile unsigned short start; //System Start Boolean
	volatile char	adx_buffer_pass[2]; //Booleans
	volatile short drv_adx_coef_1; //The (signed!) coefficient 1 the driver will use to build ADX multiplication tables.
	volatile short drv_adx_coef_2; //The (signed!) coefficient 2 the driver will use to build ADX multiplication tables.
	volatile _PCM_CTRL * pcmCtrl;
	volatile unsigned char cdda_left_channel_vol_pan; // Redbook left channel volume & pan.
	volatile unsigned char cdda_right_channel_vol_pan; // Redbook right channel volume & pan.
} sysComPara;

void ponesound_set_master_volume(unsigned short volume);
void ponesound_load_driver(uint8_t * driver_ptr, int driver_size, int master_adx_frequency);
void ponesound_load_8bit_pcm(void * pcm_ptr, int pcm_size, int loadNumber, int loadAddress, int sampleRate);
void ponesound_pcm_play(short pcmNumber, char ctrlType, char volume);
void ponesound_vblank_rq(void * work __unused);
