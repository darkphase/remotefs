/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <stdlib.h>

#include "config.h"
#include "operations/operations_rfs.h"
#include "signals.h"

static void signal_handler_client(int signal, siginfo_t *sig_info, void *ucontext_t_casted)
{
	switch (signal)
	{
	case SIGTERM:
		rfs_destroy(NULL);
		exit(0);
	}
}

void install_signal_handlers_client()
{
	install_signal_handler(SIGTERM, signal_handler_client);
}
