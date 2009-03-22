#######################################
# Build CFLAGS/LDFLAGS according to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

nssd_TARGET = rfs_nss

nssd_CFLAGS  =  $(CFLAGS_MAIN) \
                $(CFLAGS_OS) \
                $(CFLAGS_OPTS) \
                $(OPTS)

nssd_LDFLAGS = $(LDFLAGS_MAIN) \
               $(LDFLAGS_OS) \
               $(LDFLAGS_NET) \
               $(LDFLAGS_OPTS) \

#######################################
# Define target and object files
#######################################

nssd_OBJS = rfs_nss/src/dllist.o \
            rfs_nss/src/rfs_nss_server.o \
            rfs_nss/src/rfs_nss_control.o \
            rfs_nss/src/rfs_getnames.o

