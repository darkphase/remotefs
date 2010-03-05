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
              src/operations/operations.o \
              src/operations/operations_access.o \
              src/operations/operations_dirs.o \
              src/operations/operations_exports.o \
              src/operations/operations_files.o \
              src/operations/operations_io.o \
              src/operations/operations_links.o \
              src/operations/operations_rfs.o \
              src/operations/operations_read.o \
              src/operations/operations_sync.o \
              src/operations/operations_write.o \
              src/operations/operations_utils.o \
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

