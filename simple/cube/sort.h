/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef SORT_H_
#define SORT_H_

#include "cube.h"

#define OT_PRIMITIVE_BUCKETS    1
#define OT_PRIMITIVE_CNT        VDP1_CMDT_COUNT_MAX

#define OT_PRIMITIVE_BUCKET_SORT_INSERTION      0
#define OT_PRIMITIVE_BUCKET_SORT_BUBBLE         1
#define OT_PRIMITIVE_BUCKET_SORT_QUICK          2

struct ot_primitive {
        uint16_t otp_color;
        fix16_t otp_avg;
        int16_vector2_t otp_coords[4];

        TAILQ_ENTRY(ot_primitive) otp_entries;
} __aligned(32);

struct ot_primitive_bucket {
        TAILQ_HEAD(, ot_primitive) opb_bucket;
        uint32_t opb_count;
};

extern void ot_init(void);
extern void ot_primitive_add(const fix16_vector4_t *, const fix16_vector4_t *,
    uint16_t);
extern void ot_bucket_init(int32_t);
extern struct ot_primitive_bucket *ot_bucket(int32_t);
extern bool ot_bucket_empty(int32_t);

extern void ot_bucket_primitive_sort(int32_t, int32_t);

#endif /* !SORT_H */
