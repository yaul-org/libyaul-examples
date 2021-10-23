/*
 * Copyright (c) 2012-2019 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef PERF_H
#define PERF_H

#include <stdint.h>

typedef struct perf {
        uint32_t ticks;
        uint32_t max_ticks;
} perf_t;

void perf_system_init(void);

void perf_init(perf_t *perf);
void perf_start(perf_t *perf);
void perf_end(perf_t *perf);

#endif /* !PERF_H */
