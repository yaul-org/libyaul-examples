/*
 * adp68k.c
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

#include "adp68k.h"

//
// Tables must be in the first 65536 bytes of SCSP RAM
//
#include "adp68k-data.h"

SoundControlBlock screserve __attribute__((section(".control_block")));

static const uint32 isolator_addr[3] = { (uint32)&isolator, (uint32)&isolator2bit, (uint32)&isolator1bit };

static INLINE void InitADPCM(void)
{
 for(unsigned slot = 0; slot < 32; slot++)
  SCSP_SREG(slot, 0x0C) = 1 << 9;

 WaitSampleIRQ();
 WaitSampleIRQ();
 //
 //
 //
 SCSP_CREG(0x01 << 1) = (0x0 << 7) | (0 << 0);

 MADRS[0x00] = ((uint16)(uint32)&scale >> 1) + 0x0080;
 MADRS[0x01] = ((uint16)(uint32)&coeff >> 1) + 0x008;
 MADRS[0x02] = ((uint16)(uint32)&coeff >> 1) + 0x018;
 MADRS[0x03] = ((uint16)(uint32)&coeff >> 1) + 0x028;

 // Square frequency:
 MEMS[0x10] = DSP_MAKE_MEMS(-1 * 4096);

 // Square volume:
 COEF[0x10] = DSP_MAKE_COEF(0);
 COEF[0x11] = DSP_MAKE_COEF(0);

 // Saw volume:
 COEF[0x12] = DSP_MAKE_COEF(0);
 COEF[0x13] = DSP_MAKE_COEF(0);

 // Saw IIR:
 COEF[0x18] = DSP_MAKE_COEF(0);
 COEF[0x19] = DSP_MAKE_COEF(0);
 COEF[0x1A] = DSP_MAKE_COEF(0);

 for(unsigned i = 0; i < 4; i++)
 {
  // Saw freq:
  MEMS[0x14 + i] = DSP_MAKE_MEMS(0);

  // Saw operator volume:
  COEF[0x30 + i] = DSP_MAKE_COEF(0);

  // Saw feedback level:
  COEF[0x14 + i] = DSP_MAKE_COEF(0);

  // Saw modulation level:
  COEF[0x1B + i] = DSP_MAKE_COEF(0);
 }

 // CD audio volume:
 COEF[0x20] = DSP_MAKE_COEF(0); // CD left->SCSP left
 COEF[0x21] = DSP_MAKE_COEF(0); // CD left->SCSP right
 COEF[0x22] = DSP_MAKE_COEF(0);	// CD right->SCSP left
 COEF[0x23] = DSP_MAKE_COEF(0); // CD right->SCSP right

 // General constants:
 MEMS[0x1E] = DSP_MAKE_MEMS(0x1000);
 MEMS[0x1F] = DSP_MAKE_MEMS(0x800000);

 // General constants:
 COEF[0x3B] = DSP_MAKE_COEF(4096);
 COEF[0x3C] = DSP_MAKE_COEF(0);
 COEF[0x3D] = DSP_MAKE_COEF(-1);
 COEF[0x3E] = 0x8000 >> 8;
 COEF[0x3F] = 0x4000;

 WaitSampleIRQ();
 WaitSampleIRQ();

 for(unsigned i = 0; i < 128; i++)
  MPROG[i] = mprog[i] & ~DSP_TWT;

 WaitSampleIRQ();
 WaitSampleIRQ();

 for(unsigned i = 0; i < 128; i++)
  MPROG[i] = mprog[i];
 //
 //
 //
 for(unsigned adslot = 0; adslot < 8; adslot++)
 {
  const unsigned nybbles_slot = 0 + (adslot << 1);
  const unsigned selector_slot = 1 + (adslot << 1);
  const unsigned header_slot = 16 + (adslot << 1);
  const unsigned isolator_slot = 17 + (adslot << 1);

  SCSP(0x100600 + (header_slot << 1)) = 0x03FF;
  SCSP(0x100640 + (header_slot << 1)) = 0x03FF;
  //
  // The header and nybbles slots' envelope level needs to get to 0x3C0 ASAP after keying on, to prevent garbage
  // from getting into the sound path later after key off, in the time between the reconfiguration of the sound
  // slot for a new sample and the key on.
  //
  //
  SCSP_SREG(nybbles_slot, 0x04) = 0x0000; // LSA
  SCSP_SREG(nybbles_slot, 0x08) = (0x1F << 0) | (0x1F << 6) | (0x1F << 11); // Attack rate, decay rate, sustain rate
  SCSP_SREG(nybbles_slot, 0x0C) = (1 << 8); // Bypass EG and TL and ALFO
  SCSP_SREG(nybbles_slot, 0x0E) = (0x10 << 0) | (0x10 << 6) | (0x05 << 12);
  SCSP_SREG(nybbles_slot, 0x16) = (0x0 << 13) | (0x00 << 8) | (0x7 << 5) | (0x1F << 0);  // Direct send level, pan
  //
  SCSP_SREG(selector_slot, 0x02) = (uint16)(uint32)&selector; // SA
  SCSP_SREG(selector_slot, 0x04) = 0x0000; // LSA
  SCSP_SREG(selector_slot, 0x06) = 0x0008;
  SCSP_SREG(selector_slot, 0x0A) = (0x1F << 0); // Release rate
  SCSP_SREG(selector_slot, 0x0C) = (1 << 8); // Bypass EG and TL and ALFO
  SCSP_SREG(selector_slot, 0x0E) = (0x00 << 0) | (0x00 << 6) | (0x00 << 12);
  SCSP_SREG(selector_slot, 0x16) = (0x0 << 13) | (0x00 << 8) | (0x7 << 5) | (0x0F << 0);  // Direct send level, pan
  //
  SCSP_SREG(header_slot, 0x04) = 0x0000;
  SCSP_SREG(header_slot, 0x08) = (0x1F << 0) | (0x1F << 6) | (0x1F << 11); // Attack rate, decay rate, sustain rate
  SCSP_SREG(header_slot, 0x0C) = (1 << 8) | (1 << 9);
  SCSP_SREG(header_slot, 0x0E) = (0x00 << 0) | (0x00 << 6) | (0x05 << 12);
  SCSP_SREG(header_slot, 0x14) = ((0x00 + (adslot << 1)) << 3) | (0x07 << 0); // dsp mix select, mix level
  SCSP_SREG(header_slot, 0x16) = (0x0 << 13) | (0x00 << 8);  // Direct send level, pan
  //
  SCSP_SREG(isolator_slot, 0x04) = 0x0000; // LSA
  SCSP_SREG(isolator_slot, 0x06) = 0x0000; // LEA
  SCSP_SREG(isolator_slot, 0x0A) = (0x1F << 0); // Release rate
  SCSP_SREG(isolator_slot, 0x0C) = (1 << 8); // Bypass EG and TL and ALFO
  SCSP_SREG(isolator_slot, 0x10) = 0x400; // FNS
  SCSP_SREG(isolator_slot, 0x14) = ((0x01 + (adslot << 1)) << 3) | (0x07 << 0); // dsp mix select, mix level
  SCSP_SREG(isolator_slot, 0x16) = (0x0 << 13) | (0x00 << 8);  // Direct send level, pan 
 }
}

static uint16 nybbles_freg[8];
static const uint16 header_freg = 0x400 + 0x040; // header FNS

static INLINE void PlayADPCM(unsigned adslot, const uint32 addr)
{
 //const unsigned format = 1;	// 0 = 4 bits, 1 = 2 bits, 2 = 1 bit
 const unsigned nybbles_slot = 0 + (adslot << 1);
 const unsigned selector_slot = 1 + (adslot << 1);
 const unsigned header_slot = 16 + (adslot << 1);
 const unsigned isolator_slot = 17 + (adslot << 1);
 //
 const uint16 block_count = SCSP16(addr + 0);
 const uint16 loop_block = SCSP16(addr + 2);
 const unsigned format = SCSP8(addr + 4);
 const uint32 header_addr = addr + 5;
 const uint32 nybbles_addr = 1 + header_addr + block_count;
 //
 const uint16 header_lsa = loop_block;
 const uint16 nybbles_lsa = loop_block << (3 - format);
 const uint16 header_lea = block_count + (loop_block == block_count);
 const uint16 nybbles_lea = (block_count << (3 - format)) + (loop_block == block_count);

 SCSP_SREG(nybbles_slot, 0x00) = ((nybbles_addr >> 16) & 0xF) | (1 << 4) | (0x1 << 5) | (0x0 << 7) | (0x0 << 9) | (1 << 11);
 SCSP_SREG(nybbles_slot, 0x02) = nybbles_addr;	// SA
 SCSP_SREG(nybbles_slot, 0x04) = nybbles_lsa;
 SCSP_SREG(nybbles_slot, 0x06) = nybbles_lea;
 SCSP_SREG(nybbles_slot, 0x0A) = (0x1F << 0) | (0x1F << 5) | (0xE << 10) | (1 << 15); // Release rate, decay level, KRS, EG bypass
 SCSP_SREG(nybbles_slot, 0x10) = 0x400 + (0x200 >> format) + 0x4; // FNS
 //
 SCSP_SREG(selector_slot, 0x00) = (0 << 4) | (0x1 << 5) | (0x0 << 7) | (0x0 << 9) | (1 << 11);
 SCSP_SREG(selector_slot, 0x10) = ((2 - format) << 11); // FNS, Octave
 //
 SCSP_SREG(header_slot, 0x00) = ((header_addr >> 16) & 0xF) | (1 << 4) | (0x1 << 5) | (0x0 << 7) | (0x0 << 9) | (1 << 11);
 SCSP_SREG(header_slot, 0x02) = header_addr;	// SA
 SCSP_SREG(header_slot, 0x04) = header_lsa;
 SCSP_SREG(header_slot, 0x06) = header_lea;
 SCSP_SREG(header_slot, 0x0A) = (0x1F << 0) | (0x1F << 5) | (0xE << 10) | (1 << 15); // Release rate, decay level, KRS, EG bypass
 SCSP_SREG(header_slot, 0x10) = 0x400 + 0x040 + 0x4; // FNS
 //
 SCSP_SREG(isolator_slot, 0x00) = (1 << 4) | (0x1 << 5) | (0x0 << 7) | (0x0 << 9) | (1 << 11);
 SCSP_SREG(isolator_slot, 0x02) = isolator_addr[format] + (0x100 << format); // SA
 SCSP_SREG(isolator_slot, 0x0E) = (0x2F << 0) | (0x30 << 6) | ((0x09 + format) << 12);

 nybbles_freg[adslot] = 0x400 + (0x200 >> format); // nybbles FNS
}

static INLINE void Update(void)
{
 {
  volatile PSGControl* const psgc = &adp68k_scblock->psg;

  COEF[0x10] = psgc->square_volume[0];
  COEF[0x11] = psgc->square_volume[1];

  MEMS[0x10] = DSP_MAKE_MEMS(-(psgc->square_freq << 12));

  COEF[0x12] = psgc->saw_volume[0];
  COEF[0x13] = psgc->saw_volume[1];

  COEF[0x18] = psgc->saw_iir[0];
  COEF[0x19] = psgc->saw_iir[1];
  COEF[0x1A] = psgc->saw_iir[2];

  for(unsigned i = 0; i < 4; i++)
  {
   MEMS[0x14 + i] = psgc->saw_operator[i].freq;
   COEF[0x30 + i] = psgc->saw_operator[i].volume;
   COEF[0x14 + i] = psgc->saw_operator[i].fb_level;
   COEF[0x1B + i] = psgc->saw_operator[i].mod_level;
  }
 }
 //
 //
 //
 COEF[0x20] = adp68k_scblock->cd_volume[0];
 COEF[0x21] = adp68k_scblock->cd_volume[1];
 COEF[0x22] = adp68k_scblock->cd_volume[2];
 COEF[0x23] = adp68k_scblock->cd_volume[3];
 //
 //
 //
 for(unsigned ads = 0; ads < 8; ads++)
 {
  const unsigned nybbles_slot = 0 + (ads << 1);
  const unsigned selector_slot = 1 + (ads << 1);
  const unsigned header_slot = 16 + (ads << 1);
  const unsigned isolator_slot = 17 + (ads << 1);
  const unsigned mask = 0xF7 | (adp68k_scblock->adpcm[ads].action & ADP68K_ACTION_NOP);
  const unsigned egcmask = 0x7F | (adp68k_scblock->adpcm[ads].action & ADP68K_ACTION_NOP);

  SCSP_SREG_HI(header_slot, 0x0A) &= egcmask;
  SCSP_SREG_HI(nybbles_slot, 0x0A) &= egcmask;

  SCSP_SREG_HI(header_slot, 0x00) &= mask;
  SCSP_SREG_HI(nybbles_slot, 0x00) &= mask;
  SCSP_SREG_HI(selector_slot, 0x00) &= mask;
  SCSP_SREG_HI(isolator_slot, 0x00) &= mask;
 }
 SCSP_SREG(31, 0x00) |= (1 << 12);

 WaitSampleIRQ();
 WaitSampleIRQ();
 //
 //
 //
 for(unsigned ads = 0; ads < 8; ads++)
 {
  volatile ADPCMChannelControl* const chcc = &adp68k_scblock->adpcm[ads];

  COEF[(ads << 1) + 0x00] = chcc->volume[0];
  COEF[(ads << 1) + 0x01] = chcc->volume[1];
 }
 //
 //
 //
 for(unsigned ads = 0; ads < 8; ads++)
 {
  volatile ADPCMChannelControl* const chcc = &adp68k_scblock->adpcm[ads];

  if(chcc->action == ADP68K_ACTION_PLAY)
   PlayADPCM(ads, adp68k_effect_table[chcc->id]);

  chcc->action = ADP68K_ACTION_NOP;
 }
 //
 uint16 header_freg_tmp = header_freg;

 asm volatile(
	"bset #12, %1@(0x3E0)\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"
	"move.w #0x400, %1@(0x422)\n\t" "1: btst #10, %1@(0x420)\n\t" "beqs 1b\n\t"

	"move.w %0, %1@(0x210)\n\t"
	"move.w %0, %1@(0x250)\n\t"
	"move.w %0, %1@(0x290)\n\t"
	"move.w %0, %1@(0x2D0)\n\t"
	"move.w %0, %1@(0x310)\n\t"
	"move.w %0, %1@(0x350)\n\t"
	"move.w %0, %1@(0x390)\n\t"
	"move.w %0, %1@(0x3D0)\n\t"
	"move.w %2, %0\n\t"
	"move.w %0, %1@(0x010)\n\t"
	"move.w %3, %1@(0x050)\n\t"
	"move.w %4, %1@(0x090)\n\t"
	"move.w %5, %1@(0x0D0)\n\t"
	"move.w %6, %1@(0x110)\n\t"
	"move.w %7, %1@(0x150)\n\t"
	"move.w %8, %1@(0x190)\n\t"
	"move.w %9, %1@(0x1D0)\n\t"
	: "+&d"(header_freg_tmp)
	: "a"(SCSPVP(0x100000)), "a"(nybbles_freg[0]), "d"(nybbles_freg[1]), "d"(nybbles_freg[2]), "d"(nybbles_freg[3]), "d"(nybbles_freg[4]), "d"(nybbles_freg[5]), "d"(nybbles_freg[6]), "d"(nybbles_freg[7])
	: "memory", "cc");
}

void _start(void)
{
 SCSP_Init();

 SCSP_CREG_LO(0x00 << 1) = (0x0F << 0);

 InitADPCM();

 for(unsigned ads = 0; ads < 8; ads++)
 {
  volatile ADPCMChannelControl* const chcc = &adp68k_scblock->adpcm[ads];

  chcc->action = ADP68K_ACTION_NOP;
  chcc->id = 0;
  chcc->volume[0] = 0;
  chcc->volume[1] = 0;
 }

 {
  volatile PSGControl* const psgc = &adp68k_scblock->psg;

  psgc->square_volume[0] = 0;
  psgc->square_volume[1] = 0;

  psgc->saw_volume[0] = 0;
  psgc->saw_volume[1] = 0;

  psgc->saw_iir[0] = 0x4000;
  psgc->saw_iir[1] = 0;
  psgc->saw_iir[2] = 0;

  for(unsigned i = 0; i < 4; i++)
  {
   psgc->saw_operator[i].freq = 0;
   psgc->saw_operator[i].volume = 0;
   psgc->saw_operator[i].fb_level = 0;
   psgc->saw_operator[i].mod_level = 0;
  }
 }

 adp68k_scblock->cd_volume[0] = 0x4000;
 adp68k_scblock->cd_volume[1] = 0x0000;
 adp68k_scblock->cd_volume[2] = 0x0000;
 adp68k_scblock->cd_volume[3] = 0x4000;
 //
 //
 //
 SCSP_MCIPD = 1U << 5;

 for(;;)
 {
  if(SCSP_SCIPD & (1U << 5))
  {
   Update();
   SCSP_SCIRE = 1U << 5;
  }
 }
}
