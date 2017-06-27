/*
 * Copyright (c) 2012-2016 Israel Jacquez
 * See LICENSE for details.
 *
 * shazz / TRSi
 */

#ifndef FRT_H
#define FRT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <math.h>

// Clock select are defined in bits 1-0 of Time Control Register (TCR)
// 0 0 : freq/8
// 0 1 : freq/32
// 1 0 : freq/128
// 1 1 : count on rising edge
#define TIM_B_CKS0                      (0)
#define TIM_M_CKS                       (3 << TIM_B_CKS0)
#define TIM_CKS_8                       (0 << TIM_B_CKS0)
#define TIM_CKS_32                      (1 << TIM_B_CKS0)
#define TIM_CKS_128                     (2 << TIM_B_CKS0)
#define TIM_CKS_OUT                     (3 << TIM_B_CKS0)

/*
 * Init CPU Free Running Timer
 */
void tim_frt_init(uint8_t mode);

/*
 * Set CPU Free Running Timer initial value
 */
void tim_frt_set(uint16_t value);

/*
 * Get CPU Free Running Timer current value
 */
uint16_t tim_frt_get(void);

/*
 * Convert frt value to microseconds
 */
ufix16_t tim_frt_ticks_to_ms(uint16_t value);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !FRT_H */
