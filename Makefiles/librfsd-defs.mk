#######################################
# Build CFLAGS/LDFLAGS according to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

librfsd_TARGET = librfsd

librfsd_CFLAGS  = -D_FILE_OFFSET_BITS=64 \
                  $(CFLAGS_MAIN) \
                  $(CFLAGS_OS) \
                  $(CFLAGS_SO) \
                  $(CFLAGS_OPTS) \
                  $(OPTS)

librfsd_LDFLAGS = $(LDFLAGS_MAIN) \
                  $(LDFLAGS_SO) \
                  $(LDFLAGS_OS) \
                  $(LDFLAGS_NET) \
                  $(LDFLAGS_OPTS) \
                  $(OS_LIBS)

#######################################
# Define target and object files
#######################################

librfsd_OBJS = src/buffer.o \
               src/cleanup.o \
               src/command.o \
               src/crypt.o \
               src/exports.o \
               src/id_lookup.o \
               src/instance.o \
               src/keep_alive_server.o \
               src/list.o \
               src/passwd.o \
               src/path.o \
               src/rfs_acl.o \
               src/rfs_errno.o \
               src/sendrecv.o \
               src/server.o \
               src/server_handlers.o \
               src/signals.o \
               src/sockets.o \
               src/ssl.o \
               src/utils.o \
               src/md5crypt/crypt_md5.o \
               src/md5crypt/md5.o

#######################################
# Help variable for dynamic libs
#######################################

TARGET  = $(librfsd_TARGET)
TARGET_DIR = /usr/lib
