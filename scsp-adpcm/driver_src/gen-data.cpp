/*
 * gen-data.cpp
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

#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "dsp-macros.h"
#include "tables.h"

int main(int argc, char* argv[])
{
 uint64 mp[128] = { 0 };
 uint16 scale[0x100] = { 0 };

 for(unsigned ads = 0; ads < 8; ads++)
 {
  uint64* mps = &mp[ads << 4];
  //
  //
  mps[0x0] = DSP_IRA(0x20 + (ads << 1)) | DSP_ADRL | 				// ADPCM block header byte->ADRS_REG
	     DSP_XSEL(1) | DSP_ZERO | DSP_YSEL(0x1) | DSP_CRA(0x3E);		// ADPCM block header filter selection->SFT_REG

  mps[0x1] = DSP_ADRGB | DSP_TABLE | DSP_MASA(0x0) | DSP_MRT | DSP_NOFL |	// Read scale value.
	     DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 | 				// ADPCM block header filter->ADRS_REG
	     DSP_IRA(0x21 + (ads << 1)) | DSP_XSEL(1) | DSP_ZERO | DSP_YSEL(0x1) | DSP_CRA(0x3F);	// ADPCM nybble->SFT_REG
  //
  //
  //
  mps[0x2] = DSP_TWT | DSP_TWA((ads << 2) + 0x0);  				// ADPCM nybble->TEMP[adpcmslot * 4 + 0x0]

  mps[0x3] = DSP_ADRGB | DSP_TABLE | DSP_MASA(0x1) | DSP_MRT | DSP_NOFL |	// Read first filter coefficient
	     DSP_IWT | DSP_IWA(0x0);						// Scale value->MEMS[0x0]

  mps[0x4] = 0;

  mps[0x5] = DSP_ADRGB | DSP_TABLE | DSP_MASA(0x2) | DSP_MRT | DSP_NOFL | 	// Read second filter coefficient
	     DSP_IWT | DSP_IWA(0x1); 						// First filter coefficient->MEMS[0x1]

  mps[0x6] = 0;

  mps[0x7] = DSP_ADRGB | DSP_TABLE | DSP_MASA(0x3) | DSP_MRT | DSP_NOFL |	// Read third filter coefficient
	     DSP_IWT | DSP_IWA(0x2) | 	      					// Second filter cofficient->MEMS[0x2]
	     DSP_IRA(0x00) | DSP_YRL; 						// Nybble scale coefficient, MEMS[0x0]->Y_REG

  mps[0x8] = 0;

  mps[0x9] = DSP_IWT | DSP_IWA(0x3);
  //
  //
  //
  mps[0xA] = DSP_YSEL(0x2) | DSP_XSEL(0) | DSP_TRA((ads << 2) + 0x0) | DSP_ZERO |// MAC init, nybble * scale
	     DSP_IRA(0x01) | DSP_YRL;						 // First filter coefficient, MEMS[0x1]->Y_REG

  mps[0xB] = DSP_YSEL(0x2) | DSP_XSEL(0) | DSP_TRA((ads << 2) + 0x1) | DSP_BSEL(1) |	// MAC, first filter coefficient * prev0
	     DSP_IRA(0x02) | DSP_YRL;							// Second filter coefficient, MEMS[0x2]->Y_REG

  mps[0xC] = DSP_YSEL(0x2) | DSP_XSEL(0) | DSP_TRA((ads << 2) + 0x2) | DSP_BSEL(1) |	// MAC, second filter coefficient * prev1
	     DSP_IRA(0x03) | DSP_YRL;							// Third filter cofficient, MEMS[0x3]->Y_REG

  mps[0xD] = DSP_YSEL(0x2) | DSP_XSEL(0) | DSP_TRA((ads << 2) + 0x3) | DSP_BSEL(1);	// MAC, third filter coefficient * prev2

  mps[0xE] = DSP_SHFT0 | DSP_TWT | DSP_TWA((ads << 2) + 0x0);				// MAC output, decoded ADPCM sample->TEMP[ads * 4 + 0x0]

  mps[0xF] = 0;
  //
  //
  //
  if(ads & 1)
  {
   // Apply left output volume.
   mps[0x2] |= DSP_TRA((((ads & 0x6) + 0) << 2) + 0x1) | DSP_XSEL(0) | DSP_ZERO    | DSP_YSEL(0x1) | DSP_CRA((((ads & 0x6) + 0x0) << 1) + 0x0);  
   mps[0x3] |= DSP_TRA((((ads & 0x6) + 1) << 2) + 0x1) | DSP_XSEL(0) | DSP_BSEL(1) | DSP_YSEL(0x1) | DSP_CRA((((ads & 0x6) + 0x1) << 1) + 0x0);
   mps[0x4] |= DSP_TRA((ads == 5) ? 0x4C : 0x61) | DSP_XSEL(0) | DSP_BSEL(1) | DSP_YSEL(0x1) | DSP_CRA((ads == 7) ? 0x12 : (ads == 5) ? 0x10 : 0x3C); // Saw/Square
   mps[0x5] |= DSP_EWA((ads & 0x6) + 0) | DSP_EWT | DSP_SHFT0;				// Output left sample

   // Apply right output volume.
   mps[0x5] |= DSP_TRA((((ads & 0x6) + 0) << 2) + 0x1) | DSP_XSEL(0) | DSP_ZERO    | DSP_YSEL(0x1) | DSP_CRA((((ads & 0x6) + 0x0) << 1) + 0x1);  
   mps[0x6] |= DSP_TRA((((ads & 0x6) + 1) << 2) + 0x1) | DSP_XSEL(0) | DSP_BSEL(1) | DSP_YSEL(0x1) | DSP_CRA((((ads & 0x6) + 0x1) << 1) + 0x1);
   mps[0x7] |= DSP_TRA((ads == 5) ? 0x4C : 0x61) | DSP_XSEL(0) | DSP_BSEL(1) | DSP_YSEL(0x1) | DSP_CRA((ads == 7) ? 0x13 : (ads == 5) ? 0x11 : 0x3C);// Saw/square
   mps[0x8] |= DSP_EWA((ads & 0x6) + 1) | DSP_EWT | DSP_SHFT0;				// Output right sample

   if(ads == 1 || ads == 3)
   {
    mps[0x8] |= DSP_ZERO    | DSP_XSEL(1) | DSP_IRA(0x30) | DSP_YSEL(0x1) | DSP_CRA(0x20 + (ads >> 1)); // CD audio left in
    mps[0x9] |= DSP_BSEL(1) | DSP_XSEL(1) | DSP_IRA(0x31) | DSP_YSEL(0x1) | DSP_CRA(0x22 + (ads >> 1)); // CD audio right in
    mps[0xA] |= DSP_SHFT0 | DSP_EWT | DSP_EWA(0x8 + (ads >> 1));					//  -> audio out
   }
  }
  else if(ads == 0)
  {
   //
   // Square wave, parts 1 and 3.
   //
   mps[0x2] |= DSP_BSEL(0) | DSP_TRA(0x44) | DSP_XSEL(1) | DSP_IRA(0x1E) | DSP_YSEL(0x1) | DSP_CRA(0x3D);
   mps[0x3] |= DSP_TWT | DSP_TWA(0x44) | DSP_FRCL;
   mps[0x4] |= DSP_BSEL(0) | DSP_TRA(0x44) | DSP_XSEL(1) | DSP_IRA(0x10) | DSP_YSEL(0x0);
   mps[0x5] |= DSP_TWT | DSP_TWA(0x43) |
	       DSP_ZERO | DSP_XSEL(1) | DSP_IRA(0x1F) | DSP_YSEL(0x0);
   mps[0x6] |= DSP_SHFT0 | DSP_SHFT1 | DSP_FRCL;
   //
   // Saw operator 0
   //
   mps[0x6] |= DSP_ZERO | DSP_XSEL(0) | DSP_TRA(0x5D) | DSP_YSEL(0x1) | DSP_CRA(0x1B);
   mps[0x7] |= DSP_BSEL(1) | DSP_XSEL(0) | DSP_TRA(0x51) | DSP_YSEL(0x1) | DSP_CRA(0x14);
   mps[0x8] |= DSP_BSEL(1) | DSP_XSEL(1) | DSP_IRA(0x14) | DSP_YSEL(0x1) | DSP_CRA(0x3B);
   mps[0x9] |= DSP_SHFT1 | DSP_TWT | DSP_TWA(0x50);
  }
  else if(ads == 2)
  {
   //
   // Square wave, part 2, Oscillatron's Revenge
   //
   mps[0x2] |= DSP_BSEL(0) | DSP_TRA(0x48) | DSP_XSEL(1) | DSP_IRA(0x1F) | DSP_YSEL(0x0);
   mps[0x3] |= DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x48);
   mps[0x4] |= DSP_BSEL(0) | DSP_TRA(0x48) | DSP_XSEL(1) | DSP_IRA(0x1F) | DSP_YSEL(0x0);
   mps[0x5] |= DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x47);
   //
   mps[0x6] |= DSP_BSEL(0) | DSP_TRA(0x47) | DSP_XSEL(1) | DSP_IRA(0x1F) | DSP_YSEL(0x1) | DSP_CRA(0x3F); // Center square wave
   mps[0x7] |= DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x4B);
   //
   // Saw operator 1
   //
   mps[0x7] |= DSP_ZERO | DSP_XSEL(0) | DSP_TRA(0x50) | DSP_YSEL(0x1) | DSP_CRA(0x1C);
   mps[0x8] |= DSP_BSEL(1) | DSP_XSEL(0) | DSP_TRA(0x55) | DSP_YSEL(0x1) | DSP_CRA(0x15);
   mps[0x9] |= DSP_BSEL(1) | DSP_XSEL(1) | DSP_IRA(0x15) | DSP_YSEL(0x1) | DSP_CRA(0x3B);
   mps[0xA] |= DSP_SHFT1 | DSP_TWT | DSP_TWA(0x54);
  }
  else if(ads == 4)
  {
   //
   // Saw operator 2
   //
   mps[0x2] |= DSP_ZERO | DSP_XSEL(0) | DSP_TRA(0x54) | DSP_YSEL(0x1) | DSP_CRA(0x1D);
   mps[0x3] |= DSP_BSEL(1) | DSP_XSEL(0) | DSP_TRA(0x59) | DSP_YSEL(0x1) | DSP_CRA(0x16);
   mps[0x4] |= DSP_BSEL(1) | DSP_XSEL(1) | DSP_IRA(0x16) | DSP_YSEL(0x1) | DSP_CRA(0x3B);
   mps[0x5] |= DSP_SHFT1 | DSP_TWT | DSP_TWA(0x58);
   //
   // Saw operator 3
   //
   mps[0x6] |= DSP_ZERO | DSP_XSEL(0) | DSP_TRA(0x58) | DSP_YSEL(0x1) | DSP_CRA(0x1E);
   mps[0x7] |= DSP_BSEL(1) | DSP_XSEL(0) | DSP_TRA(0x5D) | DSP_YSEL(0x1) | DSP_CRA(0x17);
   mps[0x8] |= DSP_BSEL(1) | DSP_XSEL(1) | DSP_IRA(0x17) | DSP_YSEL(0x1) | DSP_CRA(0x3B);
   mps[0x9] |= DSP_SHFT1 | DSP_TWT | DSP_TWA(0x5C);
  }
  else if(ads == 6)
  {
   //
   // Saw mix and IIR
   // 
   mps[0x2] |= DSP_ZERO    | DSP_XSEL(0) | DSP_TRA(0x50) | DSP_YSEL(0x1) | DSP_CRA(0x30);
   mps[0x3] |= DSP_BSEL(1) | DSP_XSEL(0) | DSP_TRA(0x54) | DSP_YSEL(0x1) | DSP_CRA(0x31);
   mps[0x4] |= DSP_BSEL(1) | DSP_XSEL(0) | DSP_TRA(0x58) | DSP_YSEL(0x1) | DSP_CRA(0x32);
   mps[0x5] |= DSP_BSEL(1) | DSP_XSEL(0) | DSP_TRA(0x5C) | DSP_YSEL(0x1) | DSP_CRA(0x33);
   mps[0x6] |= DSP_SHFT1 | DSP_TWT | DSP_TWA(0x5D) |
	       DSP_ZERO | DSP_XSEL(0) | DSP_TRA(0x5E) | DSP_YSEL(0x1) | DSP_CRA(0x18);

   mps[0x7] |= DSP_BSEL(1) | DSP_XSEL(0) | DSP_TRA(0x5F) | DSP_YSEL(0x1) | DSP_CRA(0x19);
   mps[0x8] |= DSP_BSEL(1) | DSP_XSEL(0) | DSP_TRA(0x61) | DSP_YSEL(0x1) | DSP_CRA(0x1A);
   mps[0x9] |= DSP_SHFT0 | DSP_TWT | DSP_TWA(0x60);
  }
 }

 printf("uint64 mprog[128] =\n{\n");

 for(unsigned i = 0; i < 128; i++)
  printf(" 0x%016llx,\n", (unsigned long long)mp[i]);

 printf("};\n\n");
 //
 //
 //
 printf("uint16 scale[0x100] =\n{\n");
 for(unsigned i = 0; i < 256; i++)
 {
  unsigned sv = scale_tab[i & 0xF];

  assert(sv >= 0 && sv <= 4095);

  scale[0x080 ^ i] = sv << 3;
 }

 for(unsigned i = 0; i < 0x100; i++)
  printf(" 0x%04x,\n", scale[i]);
 printf("};\n\n");
 //
 //
 printf("uint16 coeff[0x30] =\n{\n");
 for(unsigned i = 0; i < 0x30; i++)
  printf(" 0x%04x,\n", (uint16)((uint32)filter_tab[(i ^ 0x8) & 0xF][i >> 4] << 3));
 printf("};\n\n");
 //
 //
 printf("uint16 selector[0x8] =\n{\n");
 for(unsigned i = 0; i < 8; i++)
  printf(" 0x%04x,\n", ((i + 0) & 7) << 5);
 printf("};\n\n");
 //
 //
 printf("uint8 isolator[0x200] =\n{\n");
 for(unsigned i = 0; i < 512; i++)
 {
  const uint8 bv = (i >> 1) ^ encoded_byte_xor_tab[0] ^ 0x80;

  printf(" 0x%02x,\n", ((bv >> ((i & 0x1) << 2)) & 0xF) << 4);
 }
 printf("};\n\n");
 //
 //
 printf("uint8 isolator2bit[0x400] =\n{\n");
 for(unsigned i = 0; i < 1024; i++)
 {
  static const uint8 tab[4] = { 0x30, 0x70, 0x90, 0xD0 };
  const uint8 bv = (i >> 2) ^ encoded_byte_xor_tab[1] ^ 0x80;

  printf(" 0x%02x,\n", tab[(bv >> ((i & 0x3) << 1)) & 0x3]);
 }
 printf("};\n\n");
 //
 //
 printf("uint8 isolator1bit[0x800] =\n{\n");
 for(unsigned i = 0; i < 2048; i++)
 {
  static const uint8 tab[2] = { 0x30, 0xD0 };
  const uint8 bv = (i >> 3) ^ encoded_byte_xor_tab[2] ^ 0x80;

  printf(" 0x%02x,\n", tab[(bv >> (i & 0x7)) & 0x1]);
 }
 printf("};\n\n");

 return 0;
}
