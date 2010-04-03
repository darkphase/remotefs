#######################################
# Build CFLAGS/LDFLAGS according to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

rfsd_TARGET = "rfsd"

rfsd_CFLAGS  = -D_FILE_OFFSET_BITS=64 \
                $(CFLAGS_MAIN) \
                $(CFLAGS_OS) \
                $(CFLAGS_OPTS)

rfsd_LDFLAGS = $(LDFLAGS_MAIN) \
               $(LDFLAGS_OS) \
               $(LDFLAGS_NET) \
               $(LDFLAGS_OPTS) \

#######################################
# Define target and object files
#######################################

rfsd_OBJS = src/acl/id_lookup_resolve.o \
            src/acl/handlers/getxattr.o \
            src/acl/handlers/setxattr.o \
            src/acl/server.o \
            src/acl/utils.o \
            src/nss/handlers/getnames.o \
            src/resume/cleanup.o \
            src/sendfile/read_with_sendfile.o \
            src/auth.o \
            src/buffer.o \
            src/command.o \
            src/crypt.o \
            src/exports.o \
            src/handling.o \
            src/id_lookup.o \
            src/instance.o \
            src/instance_server.o \
            src/keep_alive_server.o \
            src/list.o \
            src/passwd.o \
            src/path.o \
            src/error.o \
            src/sendrecv.o \
            src/server.o \
            src/handlers.o \
            src/handlers/chmod.o \
            src/handlers/chown.o \
            src/handlers/create.o \
            src/handlers/getattr.o \
            src/handlers/link.o \
            src/handlers/lock.o \
            src/handlers/mkdir.o \
            src/handlers/mknod.o \
            src/handlers/open.o \
            src/handlers/read.o \
            src/handlers/readdir.o \
            src/handlers/readlink.o \
            src/handlers/release.o \
            src/handlers/rename.o \
            src/handlers/rfs/auth.o \
            src/handlers/rfs/changepath.o \
            src/handlers/rfs/closeconnection.o \
            src/handlers/rfs/getexportopts.o \
            src/handlers/rfs/keepalive.o \
            src/handlers/rfs/listexports.o \
            src/handlers/rfs/request_salt.o \
            src/handlers/rmdir.o \
            src/handlers/statfs.o \
            src/handlers/symlink.o \
            src/handlers/truncate.o \
            src/handlers/write.o \
            src/handlers/unlink.o \
            src/handlers/utils.o \
            src/handlers/utime.o \
            src/handlers/utimens.o \
            src/signals.o \
            src/sockets.o \
            src/utils.o \
            src/md5crypt/crypt_md5.o \
            src/md5crypt/md5.o \
            \
            src/rfsd.o \
            src/signals_server.o \
            src/sug_server.o \
            src/scheduling.o
