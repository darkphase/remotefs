#######################################
# Build CFLAGS/LDFLAGS accordinf to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

rfs_TARGET = "rfs"

rfs_CFLAGS  = $(CFLAGS_FUSE) \
              $(CFLAGS_MAIN) \
              $(CFLAGS_OS) \
              $(CFLAGS_OPTS) \
              $(OPTS)

rfs_LDFLAGS = $(LDFLAGS_MAIN) \
              -L. -lrfs \
              $(LDFLAGS_OS) \
              $(LDFLAGS_FUSE) \
              $(LDFLAGS_NET) \
              $(LDFLAGS_PTHR) \
              $(LDFLAGS_OPTS)


#######################################
# Define target and object files
#######################################

rfs_OBJS = src/fuse_rfs.o \
           src/rfs.o \
           src/sug_client.o

