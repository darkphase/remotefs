/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SIGNALS_H
#define SIGNALS_H

/** base signals routine */

#include <signal.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** install signal handler
@param sig signal
@param signal_proc signal handler
*/
void install_signal_handler(int sig, void (*signal_proc)(int , siginfo_t *, void *));

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SIGNALS_H */
