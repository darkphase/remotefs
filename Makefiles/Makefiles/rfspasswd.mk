#######################################
# Build CFLAGS/LDFLAGS according to OS
# CC specific flags 
#######################################

CFLAGS  = $(CFLAGS_G) \
          $(CFLAGS_O) \
          $(CFLAGS_DBG) \
          $(CFLAGS_OPT)

LDFLAGS = $(LDFLAGS_G) \
          $(LDFLAGS_O) \
          $(LDFLAGS_CRYPT)

#######################################
# Define target and object files
#######################################

TARGET  = rfspasswd

OBJS =  src/rfspasswd.o \
        src/passwd.o \
        src/crypt.o \
        src/list.o \
        src/buffer.o \
        src/alloc.o \
        src/signals.o



#######################################
# Rules for compiling, ...
#######################################

include Makefiles/base.mk
