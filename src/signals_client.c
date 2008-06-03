#include "signals_client.h"

#include <signal.h>
#include <stdlib.h>

#include "config.h"
#include "sendrecv.h"
#include "signals.h"
#include "alloc.h"
#include "operations.h"

extern int g_server_socket;

void signal_handler_client(int signal, siginfo_t *sig_info, void *ucontext_t_casted)
{
	switch (signal)
	{
	case SIGTERM:
		if (g_server_socket != -1)
		{
			rfs_destroy(NULL);
		}
		mp_force_free();
		exit(0);
	}
}

void install_signal_handlers_client()
{
	install_signal_handlers(SIGTERM, signal_handler_client);
}
