/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SCHEDULING_H
#define SCHEDULING_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#ifdef WITH_SCHEDULING

#if ! (defined DARWIN || defined LINUX)
#error Scheduling is not supported for this platform
#endif

void set_scheduler(void);

#endif /* WITH_SCHEDULING */

#ifdef WITH_PAUSE
struct rfsd_instance;

void pause_rdwr(struct rfsd_instance *instance);
#endif /* WITH_PAUSE */

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SCHEDULING_H */

