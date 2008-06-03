#include "signals_server.h"

#include <signal.h>
#include <wait.h>
#include <unistd.h>
#include <stdlib.h>

#include "config.h"
#include "signals.h"
#include "alloc.h"
#include "rfsd.h"

void signal_handler_server(int signal, siginfo_t *sig_info, void *ucontext_t_casted)
{
	switch (signal)
	{	
	case SIGCHLD:
		{
		int status = -1;
		waitpid(sig_info->si_pid, &status, 1);
		
		DEBUG("child process (%d) terminated with exit code %d\n", sig_info->si_pid, status);
		}
		break;
	
	case SIGTERM:
		stop_server();
		exit(0);
		
	case SIGALRM:
		check_keep_alive();
		break;
	}
}

void install_signal_handlers_server()
{
	install_signal_handlers(SIGCHLD, signal_handler_server);
	install_signal_handlers(SIGTERM, signal_handler_server);
	install_signal_handlers(SIGALRM, signal_handler_server);
}
