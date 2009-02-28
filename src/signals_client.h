/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SIGNALS_CLIENT_H
#define SIGNALS_CLIENT_H

/** client signals installer */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** install handlers for signals handled by client */
void install_signal_handlers_client(void);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* SIGNALS_CLIENT_H */

