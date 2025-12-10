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

#include "adpcm.h"
#include <smpc/map.h>
#include <smpc/smc.h>
#include <string.h>

static void adpcm_wait_sample_irq(void)
{
 SCSP_SCIRE = 0x400;
 while(!(SCSP_SCIPD & 0x400));
}

void adpcm_load_driver(uint8_t * driver_ptr, int driver_size)
{
    smpc_smc_call(SMPC_SMC_SNDOFF);

	SCSP16(0x100400) = 0x0200;// 4Mbit mode, master volume at 0
	
	for(unsigned slot = 0; slot < 32; slot++)
	{
		SCSP_SREG(slot, 0x00) = 0x0000;
		SCSP_SREG(slot, 0x0A) = 0x001F;
	}
	SCSP_SREG(0, 0x00) = 0x1000;

	for(unsigned i = 256; i; i--)
  		adpcm_wait_sample_irq();

	for(unsigned t = 0; t < 2; t++)
 	{
		for(uint32_t i = 0x100000; i < 0x100400; i += 2)
		SCSP16(i) = 0;

		for(uint32_t i = 0x100402; i < 0x101000; i += 2)
		SCSP16(i) = 0;

		for(unsigned i = 256; i; i--)
			adpcm_wait_sample_irq();
	}

	SCSP_SCIRE = 0xFFFF;
	SCSP_MCIRE = 0xFFFF;

	memset((void*)SCSPVP(0), 0, 0x80000);//clear sound RAM
	
	memcpy((void*)SCSPVP(0), driver_ptr, driver_size);//copy driver

	smpc_smc_call(SMPC_SMC_SNDON);
	
	while(!(SCSP_MCIPD & 0x20));
		SCSP_MCIRE_LO = 0x20;
}
