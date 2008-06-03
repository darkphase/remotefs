#ifndef OPERATIONS_H
#define OPERATIONS_H

enum e_operations { op_getattr = 1 };

struct stat;
struct utimebuf;

#include <fuse.h>

struct fuse_operations rfs_operations;

void rfs_destroy(void *rfs_init_result);

int rfs_mount(const char *path);
int rfs_auth(const char *user, const char *passwd);

/// This function is called to get the attributes of a specific file.
/// @param A path to the file that we want information about. 
/// @param A struct stat that the information should be stored in. 
/// @return negated error number or 0 if everything went OK
int rfs_getattr(const char *path, struct stat *stbuf);

int rfs_opendir(const char *path, struct fuse_file_info *fi);
int rfs_releasedir(const char *path, struct fuse_file_info *fi);

/// This function is called to obtain metainformation about files in a directory (including the ".*" files).
/// @param path A path to the directory we want information about
/// @param buf buffer containing information about directory
/// @param filler pointer to fuse_fill_dir_t() function that is used to add entries to buffer
/// @param offset offset in directory entries, read http://fuse.sourceforge.net/wiki/index.php/readdir() for more information
/// @param fi A struct fuse_file_info, contains detailed information why this readdir operation was invoked.
/// @return negated error number, or 0 if everything went OK
int rfs_readdir(const char *path, void *buf, const fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int rfs_mknod(const char *path, mode_t mode, dev_t dev);
int rfs_mkdir(const char *path, mode_t mode);
int rfs_unlink(const char *path);
int rfs_rmdir(const char *path);
int rfs_rename(const char *path, const char *new_path);
int rfs_utime(const char *path, struct utimbuf *buf);

int rfs_mknod(const char *path, mode_t mode, dev_t dev);

/// This function is called to check if user is permitted to open the file using given flags.
/// @param path A path to file we want to check for opening possibility.
/// @param fi a structure containing detailed information about operation
/// @return zero if everything went OK, negeted error otherwise
int rfs_open(const char *path, struct fuse_file_info *fi);

int rfs_release(const char *path, struct fuse_file_info *fi);
int rfs_truncate(const char *path, off_t offset);

/// This function is called to read data from already opened file.
/// @param path A path to the file from which function has to read
/// @param buf buffer to which function has to write contents of the file
/// @param size both the size of buf and amount of data to read
/// @param offset offset from the beginning of file
/// @param fi detailed information about write operation, see [fuse_file_info} for more information
/// @return amount of bytes read, or negeted error number on error
int rfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
         
/// Write data to an open file
/// @param path A path to the file from which function has to read
/// @param buf buffer to which function has to write contents of the file
/// @param size both the size of buf and amount of data to read
/// @param offset offset from the beginning of file
/// @param fi detailed information about write operation, see [fuse_file_info} for more information
/// @return Write should return exactly the number of bytes requested except on error. An exception to this is when the 'direct_io' mount option is specified (see read operation).
int rfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

int rfs_statfs(const char *path, struct statvfs *buf);

int rfs_chmod(const char *path, mode_t mode);
int rfs_chown(const char *path, uid_t uid, gid_t gid);

#endif // OPERATIONS_H
