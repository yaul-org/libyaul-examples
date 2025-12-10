/*
 * dsp-macros.h
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

#ifndef GIGASANTA_DSP_MACROS_H
#define GIGASANTA_DSP_MACROS_H

#define DSP_NXADDR	((uint64)(       1) <<  0)
#define DSP_ADRGB	((uint64)(       1) <<  1)
#define DSP_MASA(v)	((uint64)((v)&0x1F) <<  2)
#define DSP_NOFL	((uint64)(       1) <<  8)
#define DSP_CRA(v)	((uint64)((v)&0x3F) <<  9)
#define DSP_BSEL(v)	((uint64)((v)&0x01) << 16)
#define DSP_ZERO	((uint64)(       1) << 17)
#define DSP_NEGB	((uint64)(       1) << 18)
#define DSP_YRL		((uint64)(       1) << 19)
#define DSP_SHFT0	((uint64)(       1) << 20)
#define DSP_SHFT1	((uint64)(       1) << 21)
#define DSP_FRCL	((uint64)(	 1) << 22)
#define DSP_ADRL	((uint64)(	 1) << 23)
#define DSP_EWA(v)	((uint64)((v)&0x0F) << 24)
#define DSP_EWT		((uint64)(       1) << 28)
#define DSP_MRT		((uint64)(       1) << 29)
#define DSP_MWT		((uint64)(	 1) << 30)
#define DSP_TABLE	((uint64)(       1) << 31)
#define DSP_IWA(v)	((uint64)((v)&0x1F) << 32)
#define DSP_IWT		((uint64)(       1) << 37)
#define DSP_IRA(v)	((uint64)((v)&0x3F) << 38)
#define DSP_YSEL(v)	((uint64)((v)&0x03) << 45)
#define DSP_XSEL(v)	((uint64)((v)&0x01) << 47)
#define DSP_TWA(v)	((uint64)((v)&0x7F) << 48)
#define DSP_TWT		((uint64)(       1) << 55)
#define DSP_TRA(v)	((uint64)((v)&0x7F) << 56)

#endif
