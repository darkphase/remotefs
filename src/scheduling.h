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

#if defined DWITH_SCHEDULING && defined DARWIN
void set_scheduler(void);

#elif defined WITH_PAUSE && defined __linux__

struct rfsd_instance;
void set_scheduler(void);
void pause_rdwr(struct rfsd_instance *instance);

#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SCHEDULING_H */
