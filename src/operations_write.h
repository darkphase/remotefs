/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPERATIONS_WRITE_H
#define OPERATIONS_WRITE_H

/** rfs write operation(s) */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

struct rfs_instance;

/** init write-behind control info */
int init_write_behind(struct rfs_instance *instance);

/** stop write-behind thread */
void kill_write_behind(struct rfs_instance *instance);

/** flush any data cached for descriptor */
int flush_write(struct rfs_instance *instance, const char *path, uint64_t descriptor);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* OPERATIONS_WRITE_H */

