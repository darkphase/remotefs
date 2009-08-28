/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#include <src/options.h>

/* this file should be compiled first for rfs_nssd and libnss_rfs */

#ifndef RFSNSS_AVAILABLE
#	error "Sorry, but rfs_nss isn't supported for this OS or with given options"
#else
int check_options_c_dummy = 0;
#endif


