/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

static void _hardware_init(void);

static void _vblank_in_handler(void);

static void _delay(uint16_t);
static bool _xchg(uint32_t, uint32_t);

static uint32_t *_cart_area = NULL;

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        cons_write("\n[1;44m      *** DRAM Cartridge Test ***       [m\n\n");

        cons_write("Initializing DRAM cartridge... ");
        dram_cartridge_init();

        cons_write("OK!\n");

        _delay(2);

        uint32_t id;
        id = dram_cartridge_id();

        if ((id != DRAM_CARTRIDGE_ID_1MIB) && (id != DRAM_CARTRIDGE_ID_4MIB)) {
                cons_write("[4;1H[2K[11CThe extended RAM\n"
                    "[11Ccartridge is not\n"
                    "[11Cinserted properly.\n"
                    "\n"
                    "[11CPlease turn off\n"
                    "[11Cpower and re-insert\n"
                    "[11Cthe extended RAM\n"
                    "[11Ccartridge.\n");
                abort();
        }

        char *buf;
        buf = (char *)malloc(1024);
        assert(buf != NULL);

        _cart_area = (uint32_t *)dram_cartridge_area();

        size_t cart_len;
        cart_len = dram_cartridge_size();
        (void)sprintf(buf, "%s DRAM Cartridge detected\n",
            ((id == DRAM_CARTRIDGE_ID_1MIB)
                ? "8-Mbit"
                : "32-Mbit"));
        cons_write(buf);

        bool passed;
        
        uint32_t x;
        for (x = 0; x < cart_len / sizeof(x); x++) {
                vdp2_tvmd_vblank_out_wait();

                passed = _xchg(x, x);

                char *result;
                result = passed ? "OK!" : "FAILED!";
                (void)sprintf(buf, "[7;1HWriting to 0x%08X (%d%%) %s\n",
                    (uintptr_t)&_cart_area[x], (int)(x / cart_len), result);
                cons_buffer(buf);

                vdp2_tvmd_vblank_in_wait();
                cons_flush();

                if (!passed) {
                        break;
                }
        }

        cons_write(passed ? "Test is complete!" : "Test was aborted!");
        free(buf);

        while (true) {
                vdp2_tvmd_vblank_out_wait();
                vdp2_tvmd_vblank_in_wait();
                vdp2_commit();
        }
}

static void
_hardware_init(void)
{
        vdp2_init();

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);

        vdp2_sprite_type_set(0);
        vdp2_sprite_priority_set(0, 0);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 3));

        scu_ic_mask_chg(IC_MASK_ALL, IC_MASK_VBLANK_IN);
        scu_ic_ihr_set(IC_INTERRUPT_VBLANK_IN, _vblank_in_handler);
        scu_ic_mask_chg(~(IC_MASK_VBLANK_IN), IC_MASK_NONE);

        cpu_intc_mask_set(0);

        vdp2_tvmd_display_set();
}

static void
_delay(uint16_t t)
{
        uint16_t frame;

        for (frame = 0; frame < (60 * t); frame++) {
                vdp2_tvmd_vblank_out_wait();
                vdp2_tvmd_vblank_in_wait();
                vdp2_commit();
        }
}

static bool
_xchg(uint32_t v, uint32_t ofs)
{
        uint32_t w;

        _cart_area[ofs] = v;
        w = _cart_area[ofs];

        return (w == v);
}

static void
_vblank_in_handler(void)
{
        dma_queue_flush(DMA_QUEUE_TAG_VBLANK_IN);
}
