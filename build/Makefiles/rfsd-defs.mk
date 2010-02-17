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
            src/acl/server.o \
            src/acl/server_handlers_acl.o \
            src/acl/utils.o \
            src/nss/server_handlers_nss.o \
            src/resume/cleanup.o \
            src/sendfile/read_with_sendfile.o \
            src/ssl/server_handlers_ssl.o \
            src/ssl/server.o \
            src/ssl/ssl.o \
            src/auth.o \
            src/buffer.o \
            src/command.o \
            src/crypt.o \
            src/exports.o \
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
            src/server_handlers.o \
            src/server_handlers_access.o \
            src/server_handlers_dirs.o \
            src/server_handlers_exports.o \
            src/server_handlers_files.o \
            src/server_handlers_io.o \
            src/server_handlers_links.o \
            src/server_handlers_read.o \
            src/server_handlers_rfs.o \
            src/server_handlers_sync.o \
            src/server_handlers_write.o \
            src/server_handlers_utils.o \
            src/signals.o \
            src/sockets.o \
            src/utils.o \
            src/md5crypt/crypt_md5.o \
            src/md5crypt/md5.o \
            \
            src/rfsd.o \
            src/signals_server.o \
            src/sug_common.o \
            src/sug_server.o \
            src/scheduling.o

