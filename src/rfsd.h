/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSD_H
#define RFSD_H

/** server interface, basically for signals */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** shutdown server */
void stop_server(void);

/** check if keep-alive has expired */
void check_keep_alive(void);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif // RFSD_H
