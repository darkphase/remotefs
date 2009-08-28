/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_LOCK_H
#define RFSNSS_LOCK_H

/** global lock routines */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct config;

int get_global_lock(struct config *config);
int release_global_lock(struct config *config);
int test_global_lock(struct config *config);

unsigned got_global_lock(const struct config *config);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_LOCK_H */

