/*
 * Copyright (c) 2012-2025 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

/*
 *
 * Ponesound 8-bit PCM demo
 *
 */

#include <yaul.h>

#include <assert.h>
#include <stdlib.h>
#include "ponesound.h"

extern uint8_t asset_sound_driver[];
extern uint8_t asset_sound_driver_end[];
extern uint8_t asset_yaul_pcm8[];
extern uint8_t asset_yaul_pcm8_end[];

static void _vblank_out_handler(void *);

static smpc_peripheral_digital_t _digital;

//only one PCM is used in the demo
#define PCM_NUMBER (0)
#define PCM_LOAD_OFFSET (0x408 + DRV_SYS_END + 0x20)

void
reset_screen(void)
{
        dbgio_printf("[H[2J");
        dbgio_printf("Press A to start 8-bit PCM\r\n");
        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();
}

int
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        //load driver binary to SMPC
        ponesound_load_driver(asset_sound_driver, asset_sound_driver_end - asset_sound_driver,ADX_MASTER_768);

        //Load 8-bit PCM sample
        ponesound_load_8bit_pcm(asset_yaul_pcm8, asset_yaul_pcm8_end - asset_yaul_pcm8,PCM_NUMBER,PCM_LOAD_OFFSET,15360);

        //Set volume
        ponesound_set_master_volume(0xF);

        //Register vblank irq for ponesound scheduler
        vdp_sync_vblank_in_set(ponesound_vblank_rq, NULL);

        reset_screen();
        
        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                if (_digital.held.button.a != 0) {
                        dbgio_printf("\r\nStarting playback...");
                        dbgio_flush();
                        ponesound_pcm_play(PCM_NUMBER, PCM_PROTECTED, 6);

                        //waiting for 1 sec
                        for (int i=0;i<60;i++)
                	{
                        	vdp2_sync();
                        	vdp2_sync_wait();
				smpc_peripheral_process();
                	}

                        reset_screen();
                }
                else
                {
                        //waiting for 1 frame
                        vdp2_sync();
                        vdp2_sync_wait();
                }
        }
}

void
user_init(void)
{
        smpc_peripheral_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
            RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp2_tvmd_display_set();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}