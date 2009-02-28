#######################################
# Build CFLAGS/LDFLAGS accordinf to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

librfs_TARGET = librfs

librfs_CFLAGS  = $(CFLAGS_FUSE) \
                 $(CFLAGS_MAIN) \
                 $(CFLAGS_OS) \
                 $(CFLAGS_SO) \
                 $(CFLAGS_OPTS) \
                 $(OPTS)

librfs_LDFLAGS = $(LDFLAGS_MAIN) \
                 $(LDFLAGS_SO) \
                 $(LDFLAGS_OS) \
                 $(LDFLAGS_FUSE) \
                 $(LDFLAGS_NET) \
                 $(LDFLAGS_PTHR) \
                 $(LDFLAGS_OPTS) \
                 $(OS_LIBS)

#######################################
# Define target and object files
#######################################

librfs_OBJS = src/acl_utils.o \
              src/attr_cache.o \
              src/buffer.o \
              src/command.o \
              src/crypt.o \
              src/data_cache.o \
              src/id_lookup.o \
              src/id_lookup_client.o \
              src/instance.o \
              src/instance_client.o \
              src/keep_alive_client.o \
              src/list.o \
			  src/nss_client.o \
			  src/nss_server.o \
              src/operations.o \
              src/operations_acl.o \
              src/operations_rfs.o \
              src/operations_read.o \
              src/operations_sync.o \
              src/operations_write.o \
              src/path.o \
              src/resume.o \
              src/error.o \
              src/sendrecv.o \
              src/signals.o \
              src/signals_client.o \
              src/sockets.o \
              src/ssl.o \
              src/utils.o \
              src/md5crypt/crypt_md5.o \
              src/md5crypt/md5.o

#######################################
# Help variable for dynamic libs
#######################################

TARGET  = $(librfs_TARGET)
