#include "signals.h"

#include <string.h>

void install_signal_handlers(int signal, void (*signal_proc)(int , siginfo_t *, void *))
{
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	
	action.sa_sigaction = signal_proc;
	action.sa_flags = SA_SIGINFO;
	
	sigaction(signal, &action, NULL);
}
