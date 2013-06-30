/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_GET_ID_H
#define RFSNSS_GET_ID_H

/** unique ids stuff */

#include <sys/types.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

uid_t get_free_uid(struct config *config, const char *user);
gid_t get_free_gid(struct config *config, const char *group);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_GET_ID_H */

