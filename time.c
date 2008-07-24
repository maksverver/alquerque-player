#include "time.h"
#include <sys/time.h>
#include <assert.h>

double started = 0, stopped = 0, stopped_time = 0;

void time_start()
{
    if (started == 0)
        started = time_now();
    if (stopped != 0)
    {
        stopped_time += time_now() - stopped;
        stopped = 0;
    }
}

void time_stop()
{
    stopped = time_now();
}

double time_used()
{
    return time_now() - started - stopped_time;
}

double time_now()
{
    struct timeval tv;
    int n;

    n = gettimeofday(&tv, 0);
    assert(n == 0);
    return tv.tv_sec + tv.tv_usec/1e6;
}
