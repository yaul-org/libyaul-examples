/*
 * sh2.h
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

#define CCR (*(volatile uint8*)0xFFFFFE92)
#define CCR_CE 0x01
#define CCR_ID 0x02
#define CCR_OD 0x04
#define CCR_TW 0x08
#define CCR_CP 0x10
#define CCR_W0 0x40
#define CCR_W1 0x80

#define SBYCR (*(volatile uint8*)0xFFFFFE91)
#define WTCSR_R (*(volatile uint8*)0xFFFFFE80)
#define WTCNT_R (*(volatile uint8*)0xFFFFFE81)

#define WRITE_WTCNT(value) (*(volatile uint16*)0xFFFFFE88 = (0x5A00 | (uint8)(value)))
#define WRITE_WTCSR(value) (*(volatile uint16*)0xFFFFFE88 = (0xA500 | (uint8)(value)))

static INLINE uint32 ReadVBR(void)
{
 uint32 ret;

 asm volatile("stc VBR,%0\n\t" : "=r"(ret));

 return ret;
}

static INLINE void WriteVBR(uint32 value)
{
 asm volatile("ldc %0,VBR\n\t" : :"r"(value) : "memory");
}

static INLINE uint32 ReadSR(void)
{
 uint32 ret;

 asm volatile("stc SR,%0\n\t" : "=r"(ret));

 return ret;
}

static INLINE void WriteSR(uint32 value)
{
 asm volatile("ldc %0,SR\n\tnop\n\t" : :"r"(value) : "memory");
}

#define SCI_SMR (*(volatile uint8*)0xFFFFFE00)
#define SCI_BRR (*(volatile uint8*)0xFFFFFE01)
#define SCI_SCR (*(volatile uint8*)0xFFFFFE02)
#define SCI_TDR (*(volatile uint8*)0xFFFFFE03)
#define SCI_SSR (*(volatile uint8*)0xFFFFFE04)
#define SCI_RDR (*(volatile uint8*)0xFFFFFE05)

#define SCI_SSR_MPBT 0x01
#define SCI_SSR_MPB  0x02
#define SCI_SSR_TEND 0x04
#define SCI_SSR_PER  0x08
#define SCI_SSR_FER  0x10
#define SCI_SSR_ORER 0x20
#define SCI_SSR_RDRF 0x40
#define SCI_SSR_TDRE 0x80

#define ICR  (*(volatile uint16_t*)0xFFFFFEE0)
#define IPRA (*(volatile uint16_t*)0xFFFFFEE2)
#define IPRB (*(volatile uint16_t*)0xFFFFFE60)

#define VCRA (*(volatile uint16_t*)0xFFFFFE62)
#define VCRB (*(volatile uint16_t*)0xFFFFFE64)
#define VCRC (*(volatile uint16_t*)0xFFFFFE66)
#define VCRD (*(volatile uint16_t*)0xFFFFFE68)
#define VCRWDT (*(volatile uint16_t*)0xFFFFFEE4)

#define TIER	(*(volatile uint8*)0xFFFFFE10)
#define FTCSR	(*(volatile uint8*)0xFFFFFE11)
#define FRCH	(*(volatile uint8*)0xFFFFFE12)
#define FRCL	(*(volatile uint8*)0xFFFFFE13)
#define OCRABH	(*(volatile uint8*)0xFFFFFE14)
#define OCRABL	(*(volatile uint8*)0xFFFFFE15)
#define TCR	(*(volatile uint8*)0xFFFFFE16)
#define TOCR	(*(volatile uint8*)0xFFFFFE17)
#define FICRH	(*(volatile uint8*)0xFFFFFE18)
#define FICRL	(*(volatile uint8*)0xFFFFFE19)

#define DMA_SAR(n)      (*(volatile uint32*)(0xFFFFFF80 + ((n) << 4)))
#define DMA_DAR(n)      (*(volatile uint32*)(0xFFFFFF84 + ((n) << 4)))
#define DMA_TCR(n)      (*(volatile uint32*)(0xFFFFFF88 + ((n) << 4)))
#define DMA_CHCR(n)     (*(volatile uint32*)(0xFFFFFF8C + ((n) << 4)))
#define DMA_VCR(n)      (*(volatile uint32*)(0xFFFFFFA0 + ((n) << 3)))
#define DMA_DRCR(n)     (*(volatile uint8*)(0xFFFFFE71 + (n)))

#define DMA_OR          (*(volatile uint32 *)0xFFFFFFB0)

static uint32 prev_time;
static uint32 time_accum;
static uint32 time_mult;

static INLINE void TimeBegin(void)
{
 time_mult = 8;
 time_accum = 0;
 TCR = 0;	// /8
 TOCR = 0;
 prev_time = FRCH << 8;
 prev_time |= FRCL << 0;
}

static INLINE void TimeBeginExt(void)
{
 time_mult = 128;
 time_accum = 0;
 TCR = 0x2;	// /128
 TOCR = 0;
 prev_time = FRCH << 8;
 prev_time |= FRCL << 0;
}

static INLINE void TimeUpdate(void)
{
 uint32 cur_time;

 // Order is important, so don't cram them into one statement!
 cur_time = FRCH << 8;
 cur_time |= FRCL << 0;

 time_accum += (cur_time - prev_time) & 0xFFFF;
 prev_time = cur_time;
}

static INLINE uint32 TimeGet(void)
{
 return time_accum * time_mult;
}


#define DVSR (*(volatile uint32*)0xFFFFFF00)
#define DVDNT (*(volatile uint32*)0xFFFFFF04)
#define DVCR (*(volatile uint32*)0xFFFFFF08)
#define VCRDIV (*(volatile uint32*)0xFFFFFF0C)
#define DVDNTH (*(volatile uint32*)0xFFFFFF10)
#define DVDNTL (*(volatile uint32*)0xFFFFFF14)
#define DVDNTH_SHADOW (*(volatile uint32*)0xFFFFFF18)
#define DVDNTL_SHADOW (*(volatile uint32*)0xFFFFFF1C)

#define BCR1  (*(volatile uint32 *)0xFFFFFFE0)
#define BCR2  (*(volatile uint32 *)0xFFFFFFE4)
#define WCR   (*(volatile uint32 *)0xFFFFFFE8)
#define MCR   (*(volatile uint32 *)0xFFFFFFEC)
#define RTCSR (*(volatile uint32 *)0xFFFFFFF0)
#define RTCNT (*(volatile uint32 *)0xFFFFFFF4)
#define RTCOR (*(volatile uint32 *)0xFFFFFFF8)
