/*
 * Copyright (c) 2012-2017 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#define ASSERT                  1
#define FIXMATH_NO_OVERFLOW     1
#define FIXMATH_NO_ROUNDING     1

#include <yaul.h>

#include <stdio.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   224

#define RGB888_TO_RGB555(r, g, b) (0x8000 | (((b) >> 3) << 10) |               \
    (((g) >> 3) << 5) | ((r) >> 3))

#define FCT     (1 << 0)
#define FCM     (1 << 1)

#define VBE     (1 << 3)

#define BEF     (1 << 0)
#define CEF     (1 << 1)

static char text_buffer[256] __unused;

static volatile uint32_t vblank_tick = 0;
static volatile uint32_t hblank_tick = 0;
static volatile uint32_t vblank_in_scanline = 0;
static volatile uint32_t vblank_out_scanline = 0;

static volatile uint32_t tick = 0;

static volatile bool vblank_in = false;
static volatile bool swap = false;
static volatile bool erased = true;

static void hblank_in_handler(irq_mux_handle_t *);
static void hardware_init(void);
static void vblank_in_handler(irq_mux_handle_t *);
static void vblank_out_handler(irq_mux_handle_t *);
static void draw_polygon(color_rgb555_t);

static void synch(void);

void
main(void)
{
        hardware_init();

        cons_init(CONS_DRIVER_VDP2, 40, 30);

        /* Turn on display */
        vdp2_tvmd_display_set();

        uint32_t idx = 0;

        color_rgb555_t colors[] = {
                COLOR_RGB555_INITIALIZER(31,  0,  0), // Red
                COLOR_RGB555_INITIALIZER( 0, 31,  0), // Green
                COLOR_RGB555_INITIALIZER( 0,  0, 31), // Blue
                COLOR_RGB555_INITIALIZER(31, 31,  0), // Yellow
                COLOR_RGB555_INITIALIZER(31,  0, 31), // Magenta
                COLOR_RGB555_INITIALIZER( 0, 31, 31)  // Cyan
        };

        while (true) {
                (void)sprintf(text_buffer, "[H[2J%08lu, %08lu", tick, vblank_tick);
                cons_buffer(text_buffer);

                draw_polygon(colors[idx]);
                idx = (idx + 1) % 6;

                /* This simulates taking a long time to process */
                volatile int y  = 0;
                for (y = 0; y < 39; y++) {
                        vdp2_tvmd_vblank_out_wait();
                        vdp2_tvmd_vblank_in_wait();
                }

                synch();
        }
}

static void
synch(void)
{
        bool vdp1_enabled = true;

        if (vdp1_enabled) {
                /* Commit VDP1 command tables to VDP1 VRAM */
                vdp1_cmdt_list_commit();
                /* Start drawing immediately */
                MEMORY_WRITE(16, VDP1(PTMR), 0x0001);

                /* Request to swap frame buffers (erase & change) */
                swap = true;

                /* Wait until a frame buffer erase & change request is made and
                 * we're at the start of V-BLANK IN */
                while (!erased && !vblank_in) {
                }

                swap = false;
        } else {
                while (!vblank_in) {
                }
        }

        /* Update tick */
        tick++;

        cons_flush();
}

static void
hardware_init(void)
{
        /* VDP2 */
        vdp2_init();

        /* VDP1 */
        vdp1_init();

        /* Disable interrupts */
        cpu_intc_disable();

        irq_mux_t *hblank_in;
        irq_mux_t *vblank_in;
        irq_mux_t *vblank_out;

        hblank_in = vdp2_tvmd_hblank_in_irq_get();
        irq_mux_handle_add(hblank_in, hblank_in_handler, NULL);

        vblank_in = vdp2_tvmd_vblank_in_irq_get();
        irq_mux_handle_add(vblank_in, vblank_in_handler, NULL);

        vblank_out = vdp2_tvmd_vblank_out_irq_get();
        irq_mux_handle_add(vblank_out, vblank_out_handler, NULL);

        /* Enable interrupts */
        cpu_intc_enable();

        vdp2_scrn_back_screen_color_set(VRAM_ADDR_4MBIT(2, 0x01FFFE),
            COLOR_RGB555(0, 0, 7));

        vdp2_tvmd_display_res_set(TVMD_INTERLACE_NONE, TVMD_HORZ_NORMAL_A,
            TVMD_VERT_224);

        MEMORY_WRITE(16, VDP1(TVMR), 0x0000);
        MEMORY_WRITE(16, VDP1(FBCR), 0x0000);
        MEMORY_WRITE(16, VDP1(PTMR), 0x0000);

        /* 1. Upload VDP1 command tables
         * 2. Start drawing immediately
         * 3. Wait until drawing is done
         * 4. Stop drawing
         * 5. Change FB
         * 6. Wait for FB change */

        /* Since display is off, we're in VBLANK-IN */
        draw_polygon(COLOR_RGB555(0, 0, 0));
        /* Commit VDP1 command tables to VDP1 VRAM */
        vdp1_cmdt_list_commit();
        /* Start drawing immediately */
        MEMORY_WRITE(16, VDP1(PTMR), 0x0001);

        /* Wait until VDP1 finishes drawing */
        while ((MEMORY_READ(16, VDP1(EDSR)) & CEF) != CEF) {
        }

        /* Idle */
        MEMORY_WRITE(16, VDP1(PTMR), 0x0000);

        /* Change FB */
        MEMORY_WRITE(16, VDP1(TVMR), 0x0000);
        MEMORY_WRITE(16, VDP1(FBCR), FCM | FCT);

        /* Wait for change of FB */
        while ((MEMORY_READ(16, VDP1(EDSR)) & CEF) == CEF) {
        }
}

static void
draw_polygon(color_rgb555_t color __unused)
{
        vdp1_cmdt_list_begin(0); {
                struct vdp1_cmdt_local_coord local_coord;

                local_coord.lc_coord.x = SCREEN_WIDTH / 2;
                local_coord.lc_coord.y = SCREEN_HEIGHT / 2;

                struct vdp1_cmdt_system_clip_coord system_clip;

                system_clip.scc_coord.x = SCREEN_WIDTH - 1;
                system_clip.scc_coord.y = SCREEN_HEIGHT - 1;

                struct vdp1_cmdt_user_clip_coord user_clip;
                user_clip.ucc_coords[0].x = 0;
                user_clip.ucc_coords[0].y = 0;
                user_clip.ucc_coords[1].x = SCREEN_WIDTH - 1;
                user_clip.ucc_coords[1].y = SCREEN_HEIGHT - 1;

                vdp1_cmdt_system_clip_coord_set(&system_clip);
                vdp1_cmdt_user_clip_coord_set(&user_clip);

                local_coord.lc_coord.x = 0;
                local_coord.lc_coord.y = 0;
                vdp1_cmdt_local_coord_set(&local_coord);

                static struct vdp1_cmdt_polygon polygon;

                polygon.cp_color = (uint16_t)color.raw;
                polygon.cp_mode.transparent_pixel = true;
                polygon.cp_mode.end_code = false;
                polygon.cp_vertex.a.x = 0;
                polygon.cp_vertex.a.y = SCREEN_HEIGHT - 1;

                polygon.cp_vertex.b.x = SCREEN_WIDTH - 1;
                polygon.cp_vertex.b.y = SCREEN_HEIGHT - 1;

                polygon.cp_vertex.c.x = SCREEN_WIDTH - 1;
                polygon.cp_vertex.c.y = 0;

                polygon.cp_vertex.d.x = 0;
                polygon.cp_vertex.d.y = 0;

                vdp1_cmdt_polygon_draw(&polygon);

                vdp1_cmdt_end();
        } vdp1_cmdt_list_end(0);
}

static void
hblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        hblank_tick++;
}

static void
vblank_in_handler(irq_mux_handle_t *irq_mux __unused)
{
        vblank_in = true;
        vblank_tick++;
        vblank_in_scanline = vdp2_tvmd_vcount_get();

        erased = false;

        /* It's assumed when swapping is requested, a batch of command
         * tables were uploaded to VDP1 VRAM.
         *
         * Waiting until CEF=1 in EDSR, display (change) the frame
         * buffer, and erase the other frame buffer.
         *
         * Upon changing frame buffers (VBE=FCM=FCT=1), CEF=0. */
        volatile uint16_t reg_edsr = MEMORY_READ(16, VDP1(EDSR));

        if (swap && ((reg_edsr & CEF) == CEF)) {
                /* Check for transfer-over (BEF=0) (how?) */

                /* V-BLANK IN erase & change at beginning of next field */
                MEMORY_WRITE(16, VDP1(TVMR), VBE);
                MEMORY_WRITE(16, VDP1(FBCR), FCM | FCT);

                erased = true;
        }
}

static void
vblank_out_handler(irq_mux_handle_t *irq_mux __unused)
{
        vblank_in = false;
        vblank_out_scanline = vdp2_tvmd_vcount_get();

        /* Disable V-BLANK-IN erase */
        MEMORY_WRITE(16, VDP1(TVMR), 0x0000);
}
