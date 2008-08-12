#######################################
# Build CFLAGS/LDFLAGS according to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

CFLAGS  = $(CFLAGS_G) \
          $(CFLAGS_O) \
          $(CFLAGS_DBG) \
          $(CFLAGS_OPT)

LDFLAGS = $(LDFLAGS_G) \
          $(LDFLAGS_O) \
          $(LDFLAGS_NET) \
          $(LDFLAGS_PTHR) \
          $(LDFLAGS_CRYPT)

#######################################
# Define target and object files
#######################################

TARGET = rfsd

OBJS =  src/rfsd.o \
        src/sendrecv.o \
        src/signals.o \
        src/command.o \
        src/buffer.o \
        src/server_handlers.o \
        src/signals_server.o \
        src/alloc.o \
        src/list.o \
        src/exports.o \
        src/passwd.o \
        src/keep_alive_server.o \
        src/crypt.o \
	src/path.o

#######################################
# Rules for compiling, ...
#######################################

include Makefiles/base.mk
