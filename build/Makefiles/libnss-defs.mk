#######################################
# Build CFLAGS/LDFLAGS accordinf to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

libnss_TARGET = $(SO_NAME_NSS)
rfs_INCLUDES= -Isrc/ -I.

libnss_CFLAGS  = $(CFLAGS_MAIN) \
                 $(CFLAGS_OS) \
                 $(CFLAGS_SO_NSS) \
                 $(CFLAGS_OPTS) \
                 $(rfs_INCLUDES)

libnss_LDFLAGS = $(LDFLAGS_MAIN) \
                 $(LDFLAGS_SO_NSS) \
                 $(LDFLAGS_OS) \
                 $(LDFLAGS_NET) \
                 $(LDFLAGS_OPTS) \
                 -L. -lrfs

#######################################
# Define target and object files
#######################################

libnss_OBJS = rfs_nss/src/check_options.o \
              rfs_nss/src/client.o \
              rfs_nss/src/client_common.o \
              rfs_nss/src/client_ent.o \
              rfs_nss/src/common.o \
              rfs_nss/src/libnss.o \
              rfs_nss/src/nss_cmd.o 

#######################################
# Help variable for dynamic libs
#######################################

