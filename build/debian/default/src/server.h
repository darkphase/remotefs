/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_SERVER_H
#define RFSNSS_SERVER_H

/** server routines */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct config;

int start_listen(uid_t uid, struct config *config);
int start_accepting(struct config *config);

int do_stop_server(struct config *config, int ret_code);
int stop_server(struct config *config, int ret_code);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_SERVER_H */

