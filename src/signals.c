/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <string.h>

#include "signals.h"
#if defined QNX
#define SA_RESTART 0
#endif

void install_signal_handler(int sig, void (*signal_proc)(int , siginfo_t *, void *))
{
	struct sigaction action = { { 0 } };
	
	action.sa_sigaction = signal_proc;
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	
	sigaction(sig, &action, NULL);
}

