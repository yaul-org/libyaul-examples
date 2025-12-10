/*
 * adp68k.h
 *
 * Copyright (C) 2021 celeriyacon - https://github.com/celeriyacon
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#ifndef ADP68K_H
#define ADP68K_H

#include "types.h"
#include "scsp.h"

// Don't change these constants.
#define ADP68K_ACTION_NOP  0x88
#define ADP68K_ACTION_PLAY 0x11
#define ADP68K_ACTION_STOP 0x01

//
// Volume of 0x4000 = 1.0
//

typedef struct
{
 uint8 action;
 uint8 id;
 uint16 volume[2];
} ADPCMChannelControl;

typedef struct
{
 uint32 freq __attribute__((aligned(4)));
 uint16 volume;	// Output volume, pre-IIR
 //
 uint16 fb_level;	// Self-feedback
 uint16 mod_level;	// Controls how much the previous operator modulates this operator.
} SawOperatorControl;

typedef struct
{
 //
 // Square wave frequency = 44100 / (2 * square_freq)
 //  Not suitable for music, but probably ok for sound effects.
 //  Obviously, don't set square_freq to 0 or your toes may explode.
 //
 uint16 square_freq;
 uint16 square_volume[2];
 
 uint16 saw_volume[2];
 uint16 saw_iir[3];

 SawOperatorControl saw_operator[4];
} PSGControl;

typedef struct
{
 ADPCMChannelControl adpcm[8];
 PSGControl psg;

 // [0]: CD left audio->left out
 // [1]: CD right audio->left out
 // [2]: CD left audio->right out
 // [3]: CD right audio->right out
 //
 // Normal:     { 0x4000, 0x0000, 0x0000, 0x4000 }
 // Force mono: { 0x2000, 0x2000, 0x2000, 0x2000 }
 //
 uint16 cd_volume[4];
} SoundControlBlock;

static volatile SoundControlBlock* const adp68k_scblock = (volatile SoundControlBlock*)SCSPVP(0x080);
static volatile uint8* const adp68k_sdbase = (volatile uint8*)SCSPVP(0x2000);
static volatile uint32* const adp68k_effect_table = (volatile uint32*)SCSPVP(0x2000);

#endif
