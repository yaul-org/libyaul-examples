/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * Shazz / TRSi
 */

#define ASSERT                  1
#define FIXMATH_NO_OVERFLOW     1
#define FIXMATH_NO_ROUNDING     1

#include <yaul.h>
#include <scu/bus/cpu/cpu/map.h>
#include <scu/bus/cpu/smpc/smpc/map.h>

#include "frt.h"

#define TCR     0x0E16
#define FRC_H   0x0E12
#define FRC_L   0x0E13

#define	SMPC_MASK_DOTSEL        0x40
#define	SBL_SMPC_MASK_DOTSEL    0x4000

typedef float float32_t;

/*
 * Init CPU Free Running Timer
 */
void tim_frt_init(uint8_t mode)
{
        // set Time Control Register to
        uint8_t reg_tcr = (MEMORY_READ(8, CPU(TCR)) & ~TIM_M_CKS) | mode;

        MEMORY_WRITE(8, CPU(TCR), reg_tcr);
}

/*
 * Set CPU Free Running Timer initial value
 */
void tim_frt_set(uint16_t value)
{
        MEMORY_WRITE(8, CPU(FRC_H), (uint8_t)(value >> 8));
        MEMORY_WRITE(8, CPU(FRC_L), (uint8_t)(value & 0xFF));
}

/*
 * Get CPU Free Running Timer current value
 */
uint16_t tim_frt_get(void)
{
        uint16_t reg_frt_h = MEMORY_READ(8, CPU(FRC_H));
        uint16_t reg_frt_l = MEMORY_READ(8, CPU(FRC_L));

        return (reg_frt_h << 8) | reg_frt_l;
}

float test_use(float x __unused)
{
        // Convert to float: ___floatsisf()
        // Uses ___mulsf3()
        /* // System clock frequency */
        /* float S __unused = 26.8741f; */
        /* // System clock period */
        /* float T __unused = 1.0f / S; */
        /* return 1.25f * x; */

        static const float system_clock_periods[] = {
                /* 320 mode, NTSC */
                1.0f / 26.8741f,
                /* 320 mode, PAL */
                1.0f / 26.6875f,
                /* 352 mode, NTSC */
                1.0f / 28.6364f,
                /* 352 mode, NTSC */
                1.0f / 28.4375f
        };

        static const float system_clock_factors[] = {
                8.0f / 1000.0f,
                32.0f / 1000.0f,
                128.0f / 1000.0f
        };

        uint32_t clock_bit = MEMORY_READ(8, CPU(TCR)) & TIM_M_CKS;
        float clk __unused = system_clock_factors[clock_bit];

        return (system_clock_periods[0] * clk);
}

/*
 * Convert frt value to microseconds
 */
ufix16_t tim_frt_ticks_to_ms(uint16_t value __unused)
{
        /*
         * The PLL oscillation frequency is in the 320 mode (NTSC:
         * 26.8741 MHz, PAL: 26.6875 MHz) and the VDP1, VDP2 and SH-2
         * are run at this frequency.  Changed to run in 352 mode (NTSC:
         * 28.6364 MHz, PAL: 28.4375 MHz) by the CKCHG352 command.
         */

        /* // System clock frequency */
        /* float S __unused = 26.8741f; */
        /* // System clock period */
        /* float T __unused = 1.0f / S; */

        /* // Possible clock factors: 8, 32, 128 */
        /* float clk __unused = (float)(8 << ((MEMORY_READ(8, CPU(TCR)) & TIM_M_CKS) << 1)); */

        /* // Overflow can occur if value is 0xFFFF. Avoid by converting to */
        /* // milliseconds first. */
        /* float v = (float)value / 1000.0f; */

        /* return fix16_from_float((clk * v) * T); */

        // ufix16_t ufix16_value __unused = (ufix16_t)0x004188F5;
        // ufix16_t period __unused = (ufix16_t)0x986;
        // ufix16_t clk __unused = (ufix16_t)0x80000;

        ufix16_t ufix16_value __unused = ufix16_from_int(value);
        ufix16_t period __unused = ufix16_from_double(1.0 / 26.8741);
        ufix16_t clk __unused = ufix16_from_int((8 << ((MEMORY_READ(8, CPU(TCR)) & TIM_M_CKS) << 1)));
        /* return ufix16_mul(ufix16_mul(ufix16_value, period),  */
        return 0;
}

/*
 * #define TIM_FRT_CNT_TO_MCR(count)\
 *   (\
 *   (((*(uint16_t *)0x25f80004 & 0x1) == 0x1) ?   // PAL ‚©?
 *    ((SYS_GETSYSCK == 0) ? (Float32)0.037470726 : (Float32)0.035164835 ) ://PAL 26,28
 *    ((SYS_GETSYSCK == 0) ? (Float32)0.037210548 : (Float32)0.03492059 )) //NT 26,28
 *    * (count) * (8 << ((TIM_PEEK_B(TIM_REG_TCR) & TIM_M_CKS) << 1)))
 */
