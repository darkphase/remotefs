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
#	include "scheduling.h"
#endif

#if defined WITH_PAUSE

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "instance_server.h"

void pause_rdwr(struct rfsd_instance *instance)
{
	struct timeval act = { 0 }; /* all variables are must be initialized */
	long dt = 0;

	gettimeofday(&act, NULL);
	if ( instance->pause.last.tv_sec == 0 )
	{
		instance->pause.last.tv_sec = act.tv_sec;
		instance->pause.last.tv_usec = act.tv_usec;
	}

	dt = ((act.tv_sec - instance->pause.last.tv_sec) * 1000000) +
	     act.tv_usec - instance->pause.last.tv_usec;
	if ( dt > 100000 && dt < 120000 )
	{
		struct timespec ts = { 0, 10000000 };
		nanosleep(&ts, NULL);
		gettimeofday(&(instance->pause.last), NULL);
	}
	else if ( dt >= 120000 )
	{
		instance->pause.last.tv_sec  = act.tv_sec;
		instance->pause.last.tv_usec = act.tv_usec;
	}
}
#endif /* WITH_PAUSE */

#ifdef SCHEDULING_AVAILABLE

#include <pthread.h>
#include <pthread_impl.h>
#include <sched.h>
#include <string.h>

void set_scheduler(void)
{
	struct sched_param param;
	memset(&param, 0, sizeof(struct sched_param));
	param.sched_priority = sched_get_priority_max(SCHED_RR);
	pthread_setschedparam(pthread_self(), SCHED_RR, &param);
}

#endif /* SCHEDULING_AVAILABLE */

#if ! (defined SCHEDULING_AVAILABLE || defined WITH_PAUSE)
int scheduling_not_used = 0;
#endif

