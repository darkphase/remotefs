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

#if (defined LINUX || defined SOLARIS || defined FREEBSD)
#	define SENDFILE_AVAILABLE
#endif

#if ((defined LINUX || defined FREEBSD) && defined WITH_ACL)
#	define ACL_AVAILABLE
#	if defined LINUX
#		define ACL_OPERATIONS_AVAILABLE
#	endif
#endif

#if (defined DARWIN && defined WITH_SCHEDULING)
#	define SCHEDULING_AVAILABLE
#else
#if (!defined DARWIN && defined WITH_SCHEDULING)
#error WITH_SCHEDULING is available for MacOS only
#endif
#endif

#endif /* OPTIONS_H */
