/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <yaul.h>

static void _vblank_in_handler(irq_mux_handle_t *);
static void _vblank_out_handler(irq_mux_handle_t *);

static void _dma_l0_end_handler(void);
static void _dma_l1_end_handler(void);
static void _dma_l2_end_handler(void);
static void _dma_illegal_handler(void);

static void _hardware_init(void);

static void _test_0(void);
static void _test_1(void);

int
main(void)
{
        _hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 28);

        scu_dma_init();

        // _test_0();
        _test_1();

        char buffer[41];
        uint32_t p;
        p = 0;
        uint32_t dir;
        dir = 1;

        while (true) {
                vdp2_tvmd_vblank_out_wait();

                cons_buffer("[1;1H");

                cons_buffer(buffer);

                memset(buffer, ' ', sizeof(buffer));
                buffer[40] = '\0';

                buffer[p >> 16] = '*';

                if ((p >> 16) == 39) {
                        dir = -1;
                        p = 38 << 16;
                } else if ((p >> 16) == 0) {
                        dir = 1;
                        p = 1 << 16;
                } else {
                        p = p + (dir * (1 << 14));
                }

                vdp2_tvmd_vblank_in_wait();
                cons_flush();
        }
}

static void
_hardware_init(void)
{
        vdp2_init();

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);

        vdp2_sprite_priority_set(0, 0);
        vdp2_sprite_priority_set(1, 0);
        vdp2_sprite_priority_set(2, 0);
        vdp2_sprite_priority_set(3, 0);
        vdp2_sprite_priority_set(4, 0);
        vdp2_sprite_priority_set(5, 0);
        vdp2_sprite_priority_set(6, 0);
        vdp2_sprite_priority_set(7, 0);

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(3, 0x01FFFE),
            COLOR_RGB555(0, 3, 15));

        irq_mux_t *vblank_in;
        vblank_in = vdp2_tvmd_vblank_in_irq_get();
        irq_mux_handle_add(vblank_in, _vblank_in_handler, NULL);

        irq_mux_t *vblank_out;
        vblank_out = vdp2_tvmd_vblank_out_irq_get();
        irq_mux_handle_add(vblank_out, _vblank_out_handler, NULL);

        /* Enable interrupts */
        cpu_intc_mask_set(0x7);

        vdp2_tvmd_display_set();
}

static void
_vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        vdp2_commit();
}

static void
_vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
}

static void __unused
_dma_illegal_handler(void)
{
        assert(false);
}

static void __unused
_dma_l0_end_handler(void)
{
}

static void __unused
_dma_l1_end_handler(void)
{
}

static void __unused
_dma_l2_end_handler(void)
{
}

static void __unused
_test_0(void)
{
        static uint32_t buffer[8] __aligned(32) = {
                0x11111111,
                0x22222222,
                0x33333333,
                0x44444444,
                0x55555555,
                0x66666666,
                0x77777777,
                0x88888888
        };

        struct dma_xfer tbl =
            DMA_MODE_XFER_INITIALIZER(32, VRAM_ADDR_4MBIT(0, 0x00000), (uint32_t)&buffer[0]);

        struct dma_level_cfg cfg = {
                .dlc_level = 0,
                .dlc_mode = DMA_MODE_DIRECT,
                .dlc_xfer = &tbl,
                .dlc_stride = DMA_STRIDE_2_BYTES,
                .dlc_update = DMA_UPDATE_NONE,
                .dlc_starting_factor = DMA_START_FACTOR_ENABLE,
                .dlc_ihr = _dma_l0_end_handler
        };

        scu_dma_level_config_set(&cfg);
        scu_dma_level_enable(0);
        scu_dma_level_start(0);
        scu_dma_level_wait(0);
}

static void __unused
_test_1(void)
{
        static uint32_t buffer[8] __aligned(32) = {
                0x11111111,
                0x10000001,
                0x10000001,
                0x10000001,
                0x10000001,
                0x10000001,
                0x10000001,
                0x11111111
        };

        static struct dma_xfer tbl[] __aligned(128) = {
                DMA_MODE_XFER_INITIALIZER(4, VRAM_ADDR_4MBIT(0, 0x00000), (uint32_t)&buffer[0]),
                DMA_MODE_XFER_INITIALIZER(4, VRAM_ADDR_4MBIT(0, 0x00004), (uint32_t)&buffer[1]),
                DMA_MODE_XFER_INITIALIZER(4, VRAM_ADDR_4MBIT(0, 0x00008), (uint32_t)&buffer[2]),
                DMA_MODE_XFER_INITIALIZER(4, VRAM_ADDR_4MBIT(0, 0x0000C), (uint32_t)&buffer[3]),
                DMA_MODE_XFER_INITIALIZER(4, VRAM_ADDR_4MBIT(0, 0x00010), (uint32_t)&buffer[4]),
                DMA_MODE_XFER_INITIALIZER(4, VRAM_ADDR_4MBIT(0, 0x00014), (uint32_t)&buffer[5]),
                DMA_MODE_XFER_INITIALIZER(4, VRAM_ADDR_4MBIT(0, 0x00018), (uint32_t)&buffer[6]),
                DMA_MODE_XFER_INITIALIZER(4, VRAM_ADDR_4MBIT(0, 0x0001C), (uint32_t)&buffer[7])
        };

        struct dma_level_cfg cfg __unused = {
                .dlc_level = 0,
                .dlc_mode = DMA_MODE_INDIRECT,
                .dlc_xfer = &tbl[0],
                .dlc_xfer_count = sizeof(tbl) / sizeof(*tbl),
                .dlc_stride = DMA_STRIDE_2_BYTES,
                .dlc_update = DMA_UPDATE_NONE,
                .dlc_starting_factor = DMA_START_FACTOR_ENABLE,
                .dlc_ihr = _dma_l0_end_handler
        };

        cons_buffer("[4;1H");

        scu_dma_level_config_set(&cfg);
        cons_buffer((char *)buffer);
        scu_dma_level_enable(0);
        scu_dma_level_start(0);
        scu_dma_level_wait(0);
}
