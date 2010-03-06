/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/*
 * Some OS has bad realtime features, in particular
 * Mac OS X so we try to set the sceduling type to
 * a real type and set the max. priority.
 */

#include "options.h"

#ifdef SCHEDULING_AVAILABLE

#include <pthread.h>
#include <pthread_impl.h>
#include <sched.h>
#include <string.h>

#include "scheduling.h"

void set_scheduler(void)
{
	struct sched_param param;
	memset(&param, 0, sizeof(struct sched_param));
	param.sched_priority = sched_get_priority_max(SCHED_RR);
	pthread_setschedparam(pthread_self(), SCHED_RR, &param);
}

#endif /* SCHEDULING_AVAILABLE */

#if ! (defined SCHEDULING_AVAILABLE)
int scheduling_not_used = 0;
#endif
