
#include <time.h>
#include <stdint.h>

static uint64_t timing_us()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)(ts.tv_nsec/1000) + (uint64_t)(ts.tv_sec)*(uint64_t)1000000;
}
