/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

/** compatibility check routines */

#ifndef CHANGELOG_H
#define CHANGELOG_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#define COMPAT_VERSION(major, minor) \
	((major) * 1000 + (minor))

/**
\return !0 if version are compatible
\see COMPAT_VERSION()
*/
int versions_compatible(unsigned my_version, unsigned their_version);
/**
\return !0 if version are compatible
\see COMPAT_VERSION()
*/
int my_version_compatible(unsigned their_version);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CHANGELOG_H */
