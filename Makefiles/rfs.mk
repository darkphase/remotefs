#######################################
# Build CFLAGS/LDFLAGS accordinf to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

rfs_CFLAGS  = -DFUSE_USE_VERSION=25 \
              $(CFLAGS_FUSE) \
              $(CFLAGS_G) \
              $(CFLAGS_O) 

rfs_LDFLAGS = $(LDFLAGS_G) \
              $(LDFLAGS_O) \
              $(LDFLAGS_FUSE) \
              $(LDFLAGS_NET) \
              $(LDFLAGS_PTHR) \
              $(LDFLAGS_CRYPT)

#######################################
# Define target and object files
#######################################

rfs_OBJS =  src/rfs.o \
            src/operations.o \
            src/signals_client.o \
            src/attr_cache.o \
            src/keep_alive_client.o \
            src/write_cache.o

# com1_OBJS also used by rfspasswd and rfsd
com1_OBJS = src/crypt.o \
            src/passwd.o \
            src/list.o \
            src/buffer.o \
            src/signals.o 
            
# com2_OBJS also used by rfsd
com2_OBJS = src/sendrecv.o \
            src/command.o \
            src/path.o \
	    src/read_cache.o \
	    src/id_lookup.o

