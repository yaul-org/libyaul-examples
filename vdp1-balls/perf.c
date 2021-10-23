#include <yaul.h>

#include <stdbool.h>

#include "perf.h"

static void _frt_ovi_handler(void);

static volatile struct {
        bool running_perf;
        uint32_t overflow_count;
} _state;

void
perf_system_init(void)
{
        cpu_frt_init(CPU_FRT_CLOCK_DIV_8);
        cpu_frt_ovi_set(_frt_ovi_handler);

        _state.running_perf = false;
        _state.overflow_count = 0;
}

void
perf_init(perf_t *perf)
{
        perf->ticks = 0;
        perf->max_ticks = 0;
}

void
perf_start(perf_t *perf __unused)
{
        assert(!_state.running_perf);

        _state.running_perf = true;

        cpu_frt_count_set(0);
        _state.overflow_count = 0;
}

void
perf_end(perf_t *perf)
{
        const uint32_t ticks_remaining = cpu_frt_count_get();
        const uint32_t overflow_ticks = _state.overflow_count * 65536;

        assert(_state.running_perf);

        _state.running_perf = false;

        perf->ticks = ticks_remaining + overflow_ticks;

        perf->max_ticks = max(perf->ticks, perf->max_ticks);
}

static void
_frt_ovi_handler(void)
{
        _state.overflow_count++;
}
