#######################################
# Build CFLAGS/LDFLAGS accordinf to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

libnss_TARGET = $(SO_NAME_NSS)

libnss_CFLAGS  = $(CFLAGS_MAIN) \
                 $(CFLAGS_OS) \
                 $(CFLAGS_SO_NSS) \
                 $(CFLAGS_OPTS) \
                 $(OPTS)

libnss_LDFLAGS = $(LDFLAGS_MAIN) \
                 $(LDFLAGS_SO_NSS) \
                 $(LDFLAGS_OS) \
                 $(LDFLAGS_NET) \
                 $(LDFLAGS_OPTS) \
                 $(OS_LIBS)

#######################################
# Define target and object files
#######################################

libnss_OBJS = rfs_nss/src/rfs_nss_client.o

#######################################
# Help variable for dynamic libs
#######################################

TARGET  = $(libnss_TARGET)

