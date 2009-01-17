/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef SUG_SERVER_H
#define SUG_SERVER_H

/** suggestions for remotefs server */

struct rfsd_instance;

/** make suggestions for server */
int suggest_server(const struct rfsd_instance *instance);

#endif /* SUG_SERVER_H */
