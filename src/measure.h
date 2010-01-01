#ifndef MEASURE_H
#define MEASURE_H

#include <sys/time.h>

#include "config.h"

#if (defined WITH_MEASURES && defined RFS_DEBUG)

#define BEGIN_MEASURE(x) struct timeval (x); gettimeofday(&x, NULL)
#define CHECKPOINT(x) { struct timeval tmp; gettimeofday(&tmp, NULL); DEBUG("checkmark at %s:%d: %ld usecs\n", __FILE__, __LINE__, (tmp.tv_sec * 1000000 + tmp.tv_usec) - (x.tv_sec * 1000000 + x.tv_usec)); }

#else

#define BEGIN_MEASURE(x)
#define CHECKPOINT(x) 

#endif /* WITH_MEASURES && RFS_DEBUG */
#endif /* MEASURE_H */

