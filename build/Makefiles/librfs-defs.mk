#######################################
# Build CFLAGS/LDFLAGS accordinf to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

librfs_TARGET = librfs

librfs_CFLAGS  = -D_FILE_OFFSET_BITS=64 \
                 $(CFLAGS_MAIN) \
                 $(CFLAGS_OS) \
                 $(CFLAGS_SO) \
                 $(CFLAGS_OPTS)

librfs_LDFLAGS = $(LDFLAGS_MAIN) \
                 $(LDFLAGS_SO) \
                 $(LDFLAGS_OS) \
                 $(LDFLAGS_NET) \
                 $(LDFLAGS_OPTS)

#######################################
# Define target and object files
#######################################

librfs_OBJS = src/acl/local_resolve.o \
              src/acl/operations_acl.o \
              src/acl/utils.o \
              src/acl/xattr_linux.o \
              src/nss/client.o \
              src/nss/operations_nss.o \
              src/nss/server.o \
              src/resume/client.o \
              src/resume/resume.o \
              src/ssl/client.o \
              src/ssl/operations_ssl.o \
              src/ssl/ssl.o \
              src/attr_cache.o \
              src/buffer.o \
              src/command.o \
              src/connect.o \
              src/crypt.o \
              src/data_cache.o \
              src/id_lookup.o \
              src/id_lookup_client.o \
              src/instance.o \
              src/instance_client.o \
              src/keep_alive_client.o \
              src/list.o \
              src/names.o \
              src/operations/chmod.o \
              src/operations/chown.o \
              src/operations/create.o \
              src/operations/getattr.o \
              src/operations/flush.o \
              src/operations/link.o \
              src/operations/list_exports.o \
              src/operations/lock.o \
              src/operations/mkdir.o \
              src/operations/mknod.o \
              src/operations/open.o \
              src/operations/operations_rfs.o \
              src/operations/read.o \
              src/operations/readdir.o \
              src/operations/readlink.o \
              src/operations/rename.o \
              src/operations/release.o \
              src/operations/rmdir.o \
              src/operations/statfs.o \
              src/operations/symlink.o \
              src/operations/synced.o \
              src/operations/truncate.o \
              src/operations/write.o \
              src/operations/utils.o \
              src/operations/utime.o \
              src/operations/utimens.o \
              src/operations/unlink.o \
              src/path.o \
              src/error.o \
              src/sendrecv.o \
              src/signals.o \
              src/signals_client.o \
              src/sockets.o \
              src/utils.o \
              src/md5crypt/crypt_md5.o \
              src/md5crypt/md5.o  \
              src/scheduling.o

#######################################
# Help variable for dynamic libs
#######################################

TARGET  = $(librfs_TARGET)

