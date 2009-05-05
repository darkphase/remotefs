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

#if defined DARWIN || defined __linux__ && defined WITH_PAUSE
void set_scheduler(void);
#endif

#if defined __linux__ && defined WITH_PAUSE
void pause_rdwr(void);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SCHEDULING_H */
