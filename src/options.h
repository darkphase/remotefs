/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef OPTIONS_H
#define OPTIONS_H

/** options availability defines */

#if (defined LINUX || defined SOLARIS || defined FREEBSD) && defined WITH_UGO
#	define RFSNSS_AVAILABLE
#endif

#if (defined LINUX || defined SOLARIS)
#	define SENDFILE_AVAILABLE
#endif

#if (defined LINUX && defined WITH_ACL)
#	define ACL_AVAILABLE
#endif

#endif /* OPTIONS_H */

