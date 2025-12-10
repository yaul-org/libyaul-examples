/*
 * smpc.h
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

#ifndef GIGASANTA_SMPC_H
#define GIGASANTA_SMPC_H

#define SMPC_PDR1 (*(volatile uint8*)0x20100075)
#define SMPC_DDR1 (*(volatile uint8*)0x20100079)

#define SMPC_PDR2 (*(volatile uint8*)0x20100077)
#define SMPC_DDR2 (*(volatile uint8*)0x2010007B)

#define SMPC_IOSEL (*(volatile uint8*)0x2010007D)
#define SMPC_EXLE (*(volatile uint8*)0x2010007F)

#define SMPC_COMREG (*(volatile uint8*)0x2010001F)
#define SMPC_SR (*(volatile uint8*)0x20100061)
#define SMPC_SF (*(volatile uint8*)0x20100063)

#define SMPC_IREG(n) (*(volatile uint8*)(0x20100001 + ((n) << 1)))
#define SMPC_OREG(n) (*(volatile uint8*)(0x20100021 + ((n) << 1)))

#define SMPC_CMD_MSHON 0x00
#define SMPC_CMD_SSHON 0x02
#define SMPC_CMD_SSHOFF 0x03

#define SMPC_CMD_SNDON 0x06
#define SMPC_CMD_SNDOFF 0x07

#define SMPC_CMD_CDON 0x08
#define SMPC_CMD_CDOFF 0x09

#define SMPC_CMD_SYSRES 0x0D

#define SMPC_CMD_CKCHG352 0x0E
#define SMPC_CMD_CKCHG320 0x0F

#define SMPC_CMD_INTBACK 0x10
#define SMPC_CMD_SETTIME 0x16
#define SMPC_CMD_SETSMEM 0x17

#define SMPC_CMD_NMIREQ 0x18
#define SMPC_CMD_RESENAB 0x19
#define SMPC_CMD_RESDISA 0x1A

static INLINE void SMPC_SoundOn(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_SNDON;
 while(SMPC_SF & 0x1);
}

static INLINE void SMPC_SoundOff(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_SNDOFF;
 while(SMPC_SF & 0x1);
}

static INLINE void SMPC_CDOn(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_CDON;
 while(SMPC_SF & 0x1);
}

static INLINE void SMPC_CDOff(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_CDOFF;
 while(SMPC_SF & 0x1);
}

static INLINE void SMPC_SSHOn(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_SSHON;
 while(SMPC_SF & 0x1);
}

static INLINE void SMPC_SSHOff(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_SSHOFF;
 while(SMPC_SF & 0x1);
}

#endif
