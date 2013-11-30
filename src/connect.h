/*
remotefs file system
See the file AUTHORS for copyright information.

This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef CONNECT_H
#define CONNECT_H

/** socket send/recv and connect/disconnect routines */

#include "sendrecv.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

int rfs_connect(rfs_sendrecv_info_t *info, const char *ip, unsigned port, unsigned force_ipv4, unsigned force_ipv6);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CONNECT_H */
