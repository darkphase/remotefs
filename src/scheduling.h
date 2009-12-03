/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SCHEDULING_H
#define SCHEDULING_H

#include "options.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#ifdef SCHEDULING_AVAILABLE

/** set real-time scheduling and max priority to current thread */
void set_scheduler(void);

#endif /* SCHEDULING_AVAILABLE */

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SCHEDULING_H */

