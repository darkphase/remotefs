/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef CRYPT_H
#define CRYPT_H

/** password encryption routine */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** get md5-hash of password, don't forget to free() returned value 
\return md5 hash of password encrypted with salt without leading $1$ */
char *passwd_hash(const char *password, const char *salt);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* CRYPT_H */

