
#include "uv-posix-polyfills.h"
#include <time.h>
#include <stdlib.h>

uint64_t uv__hrtime(uv_clocktype_t type)
{
    struct timespec ts;
    clockid_t clk_id;

    /* UV_CLOCK_FAST is allowed to be imprecise, MONOTONIC_FAST is faster on FreeBSD */
    if (type == UV_CLOCK_FAST)
        clk_id = CLOCK_MONOTONIC_FAST;
    else
        clk_id = CLOCK_MONOTONIC;

    if (clock_gettime(clk_id, &ts) != 0)
        abort();

    return (uint64_t)ts.tv_sec * (uint64_t)1e9 + (uint64_t)ts.tv_nsec;
}
