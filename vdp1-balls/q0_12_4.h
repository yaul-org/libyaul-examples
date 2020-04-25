#ifndef Q0_12_4_H
#define Q0_12_4_H

#include <stdint.h>
#include <stddef.h>

/* Q0.12.4 format */
typedef int16_t q0_12_4_t;

#define Q0_12_4(v)      ((q0_12_4_t)(((float)(v)) * 16.0f))
#define Q0_12_4_INT(v)  ((((q0_12_4_t)(v)) >> 4))
#define Q0_12_4_FRAC(v) (((q0_12_4_t)(v)) & 0x0F)

#endif /* Q0_12_4_H */
