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
 *
 * On Mac OS X this can be done without problem
 * on other the owner must be the super user.
 *
 * For Linux we don't need such settings, Linux has
 * good capabities for us.
 *
 * If we can't set the priority, the program will work
 * the only problem is that the maximal performance can't
 * be always reached, so we ignore possibly errors.
 *
 * On a Linux system as a router the read/write calls
 * require aboout 100 % of the CPU time. In oider to
 * allow other application to run we set scheduling to
 * Real Time with Round Robin and set the rfs server
 * to the sleeping state for 10 ms if the time elapsed
 * agter the last read or write call was greater thsn
 * 100 ms. This will limit the CPU resource for refs
 * to approximately 89 % and allow other application
 * to run while as read oe write call was issued.
 *
 */

#if defined __linux__ && defined MAKE_PAUSE

#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "scheduling.h"

void set_scheduler(void)
{
	struct sched_param param;
	int priority = sched_get_priority_max(SCHED_RR);
	sched_getparam(0,&param);
	param.sched_priority = priority;
	sched_setscheduler(0,SCHED_RR,&param);
}

void pause_rdwr(void)
{
	static struct timeval last = { 0, 0 };
	struct timeval act;
	long dt = 0;

	gettimeofday(&act, NULL);
	if ( last.tv_sec == 0 )
	{
		last.tv_sec = act.tv_sec;
		last.tv_usec = act.tv_usec;
	}

	dt = ((act.tv_sec - last.tv_sec) * 1000000) +
	     act.tv_usec - last.tv_usec;
	if ( dt > 100000 && dt < 200000 )
	{
		gettimeofday(&last, NULL);
		usleep(10000);
	}
	else if ( dt > 200000 )
	{
		last.tv_sec = act.tv_sec;
		last.tv_usec = act.tv_usec;
	}
}

#elif defined DARWIN

# include <pthread.h>
# include <pthread_impl.h>
# include <sched.h>
# include <string.h>

void set_scheduler(void)
{
	struct sched_param param;
	memset(&param, 0, sizeof(struct sched_param));
	param.sched_priority = sched_get_priority_max(SCHED_RR);
	pthread_setschedparam(pthread_self(), SCHED_RR, &param);
}

#endif
