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

/** set socket timeout */
int setup_socket_timeout(int socket, const int timeout);

/** set socket buffer */
int setup_socket_buffer(int socket, const int size);

/** set socket to reuse address */
int setup_socket_reuse(int socket, const char reuse);

/** set socket pid */
int setup_soket_pid(int socket, const pid_t pid);

int setup_socket_ndelay(int socket, const char nodelay);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // SOCKETS_H

