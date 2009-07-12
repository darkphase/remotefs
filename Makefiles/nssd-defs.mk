#######################################
# Build CFLAGS/LDFLAGS according to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

nssd_TARGET = rfs_nssd
rfs_INCLUDES= -Isrc/

nssd_CFLAGS  =  $(CFLAGS_MAIN) \
                $(CFLAGS_OS) \
                $(CFLAGS_OPTS) \
				$(rfs_INCLUDES)

nssd_LDFLAGS = $(LDFLAGS_MAIN) \
               $(LDFLAGS_OS) \
               $(LDFLAGS_NET) \
               $(LDFLAGS_PTHR) \
               $(LDFLAGS_OPTS) \
			   -L. -lrfs

#######################################
# Define target and object files
#######################################

nssd_OBJS = rfs_nss/src/client_common.o \
			rfs_nss/src/client_for_server.o \
			rfs_nss/src/cookies.o \
			rfs_nss/src/common.o \
			rfs_nss/src/config.o \
			rfs_nss/src/get_id.o \
			rfs_nss/src/global_lock.o \
			rfs_nss/src/processing.o \
			rfs_nss/src/processing_common.o \
			rfs_nss/src/processing_ent.o \
			rfs_nss/src/manage_server.o \
			rfs_nss/src/nss_cmd.o \
			rfs_nss/src/rfs_nssd.o \
			rfs_nss/src/server.o 

