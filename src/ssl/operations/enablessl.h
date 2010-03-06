/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifdef WITH_SSL

#ifndef OPERATIONS_SSL_ENABLESSL_H
#define OPERATIONS_SSL_ENABLESSL_H

struct rfs_instance;
int rfs_enablessl(struct rfs_instance *instance, unsigned show_errors);

#endif /* OPERATIONS_SSL_ENABLESSL_H */
#endif /* WITH_SSL */
