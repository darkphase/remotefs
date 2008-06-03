#include "signals_server.h"

#include <signal.h>
#include <wait.h>
#include <unistd.h>
#include <stdlib.h>

#include "config.h"
#include "signals.h"
#include "alloc.h"
#include "rfsd.h"

extern int g_client_socket;

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
		server_close_connection(g_client_socket);
		exit(0);
	}
}

void install_signal_handlers_server()
{
	install_signal_handlers(SIGCHLD, signal_handler_server);
	install_signal_handlers(SIGTERM, signal_handler_server);
}
