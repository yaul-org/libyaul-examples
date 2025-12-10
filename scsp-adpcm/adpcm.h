/*
 * Copyright (c) 2012-2025 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdint.h>
#include "yaul.h"

#define SCSPVP(offset) ((volatile void*)(0x25A00000 + (offset)))
#define SCSP16(offset) (*(volatile uint16_t*)SCSPVP(offset))
#define SCSP8(offset) (*(volatile uint8_t*)SCSPVP(offset))
#define SCSP_MEM(offset) SCSP16(offset)

#define SCSP_SREG(slot, offset)    SCSP16(0x100000 + (slot) * 0x20 + (offset))
#define SCSP_SREG_HI(slot, offset)  SCSP8(0x100000 + (slot) * 0x20 + (offset))
#define SCSP_SREG_LO(slot, offset)  SCSP8(0x100001 + (slot) * 0x20 + (offset))

#define SCSP_CREG(offset)    SCSP16(0x100400 + (offset))
#define SCSP_CREG_HI(offset)  SCSP8(0x100400 + (offset))
#define SCSP_CREG_LO(offset)  SCSP8(0x100401 + (offset))

#define SCSP_SCIEB SCSP_CREG(0x0F << 1)
#define SCSP_SCIPD SCSP_CREG(0x10 << 1)
#define SCSP_SCIRE SCSP_CREG(0x11 << 1)

#define SCSP_MCIEB SCSP_CREG(0x15 << 1)
#define SCSP_MCIPD SCSP_CREG(0x16 << 1)
#define SCSP_MCIRE SCSP_CREG(0x17 << 1)

#define SCSP_SCIEB_HI SCSP_CREG_HI(0x0F << 1)
#define SCSP_SCIPD_HI SCSP_CREG_HI(0x10 << 1)
#define SCSP_SCIRE_HI SCSP_CREG_HI(0x11 << 1)

#define SCSP_MCIEB_HI SCSP_CREG_HI(0x15 << 1)
#define SCSP_MCIPD_HI SCSP_CREG_HI(0x16 << 1)
#define SCSP_MCIRE_HI SCSP_CREG_HI(0x17 << 1)

#define SCSP_SCIEB_LO SCSP_CREG_LO(0x0F << 1)
#define SCSP_SCIPD_LO SCSP_CREG_LO(0x10 << 1)
#define SCSP_SCIRE_LO SCSP_CREG_LO(0x11 << 1)

#define SCSP_MCIEB_LO SCSP_CREG_LO(0x15 << 1)
#define SCSP_MCIPD_LO SCSP_CREG_LO(0x16 << 1)
#define SCSP_MCIRE_LO SCSP_CREG_LO(0x17 << 1)

void adpcm_load_driver(uint8_t * driver_ptr, int driver_size);
