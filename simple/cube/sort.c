/*
 * Copyright (c) 2012-2014 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include "cube.h"

struct ot_primitive_bucket {
        struct ot_primitive_head opb_bucket;
        uint32_t opb_count;
};

/* OT */
static struct ot_primitive_bucket ot_primitive_bucket_pool[OT_PRIMITIVE_BUCKETS];
static struct ot_primitive ot_primitive_pool[OT_PRIMITIVE_CNT];

static uint32_t ot_primitive_pool_idx;

void
ot_init(void)
{
        int32_t bucket_idx;
        for (bucket_idx = 0; bucket_idx < OT_PRIMITIVE_BUCKETS; bucket_idx++) {
                ot_bucket_init(bucket_idx);
        }

        ot_primitive_pool_idx = 0;
}

void
ot_bucket_init(int32_t idx)
{
        struct ot_primitive_bucket *opb;
        opb = &ot_primitive_bucket_pool[idx & (OT_PRIMITIVE_BUCKETS - 1)];

        TAILQ_INIT(&opb->opb_bucket);
        opb->opb_count = 0;
}

struct ot_primitive_head *
ot_bucket(int32_t idx)
{
        return &ot_primitive_bucket_pool[idx].opb_bucket;
}

bool
ot_bucket_empty(int32_t idx)
{
        return TAILQ_EMPTY(&ot_primitive_bucket_pool[idx].opb_bucket);
}

void
ot_primitive_add(const fix16_vector4_t *vertex_mv,
    const fix16_vector4_t *vertex_projected, uint16_t color)
{
        struct ot_primitive *otp;
        otp = &ot_primitive_pool[ot_primitive_pool_idx];

        ot_primitive_pool_idx++;
        ot_primitive_pool_idx &= OT_PRIMITIVE_CNT - 1;

        fix16_t avg;
        avg = fix16_mul(fix16_add(
                    fix16_add(vertex_mv[0].z, vertex_mv[1].z),
                    fix16_add(vertex_mv[2].z, vertex_mv[3].z)),
            F16(1.0f / 4.0f));

        otp->otp_color = color;
        otp->otp_avg = avg;

        fix16_t half_width;
        half_width = F16((float)SCREEN_WIDTH / 2.0f);

        fix16_t half_height;
        half_height = F16((float)-SCREEN_HEIGHT / 2.0f);

        otp->otp_coords[0].x = fix16_to_int(
                fix16_mul(vertex_projected[0].x, half_width));
        otp->otp_coords[0].y = fix16_to_int(
                fix16_mul(vertex_projected[0].y, half_height));
        otp->otp_coords[1].x = fix16_to_int(
                fix16_mul(vertex_projected[1].x, half_width));
        otp->otp_coords[1].y = fix16_to_int(
                fix16_mul(vertex_projected[1].y, half_height));
        otp->otp_coords[2].x = fix16_to_int(
                fix16_mul(vertex_projected[2].x, half_width));
        otp->otp_coords[2].y = fix16_to_int(
                fix16_mul(vertex_projected[2].y, half_height));
        otp->otp_coords[3].x = fix16_to_int(
                fix16_mul(vertex_projected[3].x, half_width));
        otp->otp_coords[3].y = fix16_to_int(
                fix16_mul(vertex_projected[3].y, half_height));
        /* Screen coordinates */

        fix16_t abs_avg;
        abs_avg = fix16_abs(avg);

        int32_t idx;
        /* XXX */
        idx = fix16_to_int(abs_avg);

        struct ot_primitive_bucket *opb;
        opb = &ot_primitive_bucket_pool[idx & (OT_PRIMITIVE_BUCKETS - 1)];

        TAILQ_INSERT_HEAD(&opb->opb_bucket, otp, otp_entries);
        opb->opb_count++;
}

void
ot_bucket_primitive_sort(int32_t idx, int32_t type)
{
        struct ot_primitive *head;
        head = NULL;

        struct ot_primitive *safe;
        struct ot_primitive *otp;

        if (type == OT_PRIMITIVE_BUCKET_SORT_INSERTION) {
                for (otp = TAILQ_FIRST(&ot_primitive_bucket_pool[idx].opb_bucket);
                     (otp != NULL) && (safe = TAILQ_NEXT(otp, otp_entries), 1);
                     otp = safe) {
                        struct ot_primitive *otp_current;
                        otp_current = otp;
                        if ((head == NULL) || (otp_current->otp_avg > head->otp_avg)) {
                                TAILQ_NEXT(otp_current, otp_entries) = head;
                                head = otp_current;
                                TAILQ_FIRST(&ot_primitive_bucket_pool[idx].opb_bucket) = head;
                                continue;
                        }

                        struct ot_primitive *otp_p;
                        for (otp_p = head; otp_p != NULL; ) {
                                struct ot_primitive **otp_p_next;
                                otp_p_next = &TAILQ_NEXT(otp_p, otp_entries);

                                if ((*otp_p_next == NULL) ||
                                    (otp_current->otp_avg > (*otp_p_next)->otp_avg)) {
                                        TAILQ_NEXT(otp_current, otp_entries) =
                                            *otp_p_next;
                                        *otp_p_next = otp_current;
                                        break;
                                }

                                otp_p = *otp_p_next;
                        }
                }
        } else if (type == OT_PRIMITIVE_BUCKET_SORT_BUBBLE) {
                bool swapped;
                do {
                        swapped = false;

                        struct ot_primitive *otp_k __unused;
                        otp_k = NULL;

                        head = TAILQ_FIRST(&ot_primitive_bucket_pool[idx].opb_bucket);
                        for (otp = head;
                             (otp != NULL) && (safe = TAILQ_NEXT(otp, otp_entries), 1);
                             otp = safe) {
                                struct ot_primitive *otp_i;
                                otp_i = otp;

                                struct ot_primitive *otp_j;
                                otp_j = TAILQ_NEXT(otp, otp_entries);

                                if (otp_j == NULL) {
                                        continue;
                                }

                                if (otp_j->otp_avg > otp_i->otp_avg) {
                                        if (otp_k == NULL) {
                                                TAILQ_FIRST(&ot_primitive_bucket_pool[idx].opb_bucket) =
                                                    otp_j;
                                        } else {
                                                TAILQ_NEXT(otp_k, otp_entries) =
                                                    otp_j;
                                        }

                                        TAILQ_NEXT(otp_i, otp_entries) =
                                            TAILQ_NEXT(otp_j, otp_entries);
                                        TAILQ_NEXT(otp_j, otp_entries) = otp_i;

                                        swapped = true;
                                }

                                otp_k = otp_i;
                        }
                } while (swapped);
        } else if (type == OT_PRIMITIVE_BUCKET_SORT_QUICK) {
        }
}
