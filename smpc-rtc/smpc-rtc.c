/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <yaul.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BACK_SCREEN VDP2_VRAM_ADDR(3, 0x1FFFE)

static void _vblank_out_handler(void *work);

static const char *_week_days_strings[] = {
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday"
};

static const char *_month_strings[] = {
        NULL,
        "January",
        "February",
        "March",
        "April",
        "May",
        "June",
        "July",
        "August",
        "September",
        "October",
        "November",
        "December"
};

static const char *_number_suffix_string[] = {
        "th",
        "st",
        "nd",
        "rd",
        "th",
        "th",
        "th",
        "th",
        "th",
        "th"
};

int
main(void)
{
        dbgio_init();
        dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
        dbgio_dev_font_load();

        vdp2_sync();
        vdp2_sync_wait();

        const smpc_time_t * const time = smpc_rtc_time_get();
        smpc_time_dec_t time_dec;

        /* Wait until the SMPC peripheral INTBACK command has been issued at
         * least once */
        vdp2_tvmd_vblank_in_wait();
        vdp2_tvmd_vblank_out_wait();
        smpc_peripheral_process();
        smpc_rtc_time_bcd_from(time, &time_dec);
        /* Use SMPC RTC date as a 32-bit seed for the default PRNG */
        const uint32_t seed = ((uint32_t)time_dec.month * 2629800UL) +
                              ((uint32_t)time_dec.day * 86400UL) +
                              ((uint32_t)time_dec.hours * 3600UL) +
                              ((uint32_t)time_dec.minutes * 60UL) +
                               (uint32_t)time_dec.seconds;

        dbgio_printf("[HUsing seed: %lu\n", seed);

        srand(seed);

        while (true) {
                smpc_peripheral_process();

                dbgio_printf("[3;1H[1J");


                dbgio_printf("%i/%i/%i %02i:%02i:%02i\n",
                    time_dec.day,
                    time_dec.month,
                    time_dec.year,
                    time_dec.hours,
                    time_dec.minutes,
                    time_dec.seconds);

                dbgio_printf("%s, the %i%s of %s, %i\n",
                    _week_days_strings[time_dec.week_day],
                    time_dec.day,
                    /* Use the ones digit from BCD */
                    _number_suffix_string[time->day & 0xF],
                    _month_strings[time_dec.month],
                    time_dec.year);

                dbgio_printf("\nRandom value, %lu\n", (rand() & 15));

                dbgio_flush();
                vdp2_sync();
                vdp2_sync_wait();
        }

        return 0;
}

void
user_init(void)
{
        smpc_peripheral_init();

        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_240);

        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 0, 7, 7));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        vdp2_tvmd_display_set();
}

static void
_vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}
