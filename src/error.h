/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef ERRNO_H
#define ERRNO_H

/** errno defines and utilities */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** convert system errno to rfs errno in network byteorder */
int hton_errno(int host_errno);

/** convert rfs errno to system errno in host byteorder */
int ntoh_errno(int net_errno);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* ERRNO_H */
