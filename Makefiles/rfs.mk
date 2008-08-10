#######################################
# Build CFLAGS/LDFLAGS accordinf to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

CFLAGS  = -DFUSE_USE_VERSION=25 \
          $(CFLAGS_FUSE) \
          $(CFLAGS_G) \
          $(CFLAGS_O) \
          $(CFLAGS_DBG) \
          $(CFLAGS_OPT)

LDFLAGS = $(LDFLAGS_G) \
          $(LDFLAGS_O) \
          $(LDFLAGS_FUSE) \
          $(LDFLAGS_NET) \
          $(LDFLAGS_PTHR) \
          $(LDFLAGS_CRYPT)

#######################################
# Define target and object files
#######################################

TARGET = rfs
OBJS =  src/rfs.o \
        src/operations.o \
        src/buffer.o \
        src/command.o \
        src/sendrecv.o \
        src/signals.o \
        src/signals_client.o \
        src/alloc.o \
        src/attr_cache.o \
        src/list.o \
        src/passwd.o \
        src/crypt.o \
        src/keep_alive_client.o \
        src/write_cache.o \
        src/read_cache.o

#######################################
# Rules for compiling, ...
#######################################

include Makefiles/base.mk
