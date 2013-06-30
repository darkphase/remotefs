/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef RFSNSS_MAINTENANCE_H
#define RFSNSS_MAINTENANCE_H

/** server maintenance */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct config;

/** return 0 if thread successfully started */
int start_maintenance_thread(struct config *config);
void stop_maintenance_thread(struct config *config);

/** return 0 if locked, -1 on error */
int maintenance_lock(struct config *config);

/** return 0 if locked, -1 on error */
int maintenance_unlock(struct config *config);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* RFSNSS_MAINTENANCE_H */

