/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef SOLARIS
#       include <wait.h>
#else
#       include <sys/wait.h>
#endif
#include <stdlib.h>

#include "config.h"
#include "rfsd.h"
#include "server.h"
#include "signals.h"

static void signal_handler_server(int signal, siginfo_t *sig_info, void *ucontext_t_casted)
{
	switch (signal)
	{	
	case SIGCHLD:
		{
		int status = -1;

		while (waitpid(-1, &status, WNOHANG) > 0)
		{
			if (WIFEXITED(status))
			{
				DEBUG("child process terminated with exit code %d\n", WEXITSTATUS(status));
			}
			else if (WIFSIGNALED(status))
			{
				DEBUG("child process killed by signal %d\n", WTERMSIG(status));
			}
		}

		}
		break;
	
	case SIGHUP:
	case SIGTERM:
	case SIGINT:
		stop_server(&rfsd_instance);
		exit(0);
		
	case SIGALRM:
		check_keep_alive(&rfsd_instance);
		break;
		
	case SIGPIPE:
		break;
	}
}

void install_signal_handlers_server()
{
	install_signal_handler(SIGCHLD, signal_handler_server);
	install_signal_handler(SIGTERM, signal_handler_server);
	install_signal_handler(SIGINT, signal_handler_server);
	install_signal_handler(SIGALRM, signal_handler_server);
	install_signal_handler(SIGPIPE, signal_handler_server);
	install_signal_handler(SIGHUP, signal_handler_server);
}

