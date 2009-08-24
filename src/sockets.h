/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SOCKETS_H
#define SOCKETS_H

/** client interface, just config for now */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** set socket to reuse address */
int setup_socket_reuse(int socket, const char reuse);

/** set socket pid */
int setup_soket_pid(int socket, const pid_t pid);

int setup_socket_ndelay(int socket, const char nodelay);

#ifdef WITH_IPV6
/** set socket for usage with IPv6 only */
int setup_socket_ipv6_only(int socket);
#endif

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // SOCKETS_H

