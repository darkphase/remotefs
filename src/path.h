/*
remotefs file system
See the file AUTHORS for copyright information.
	
This program can be distributed under the terms of the GNU GPL.
See the file LICENSE.
*/

#ifndef PATH_H
#define PATH_H

/** *rfs related* os path routines */

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/** used in rfs_readdir()
@param full_path[FILENAME_MAX] join result
@param path directory part
@param filename filename part
@return 0 if joined, -1 on error, 1 if join isn't needed (e.g. on filename == "..")
*/
int path_join(char *full_path, size_t max_len, const char *path, const char *filename);

#if defined (__cplusplus) || defined (c_plusplus)
}
#endif

#endif /* PATH_H */

