#######################################
# Build CFLAGS/LDFLAGS according to OS
# CC specific flags 
#######################################

rfspasswd_TARGET = "rfspasswd"

rfspasswd_CFLAGS  = $(CFLAGS_G) \
	$(CFLAGS_O)

rfspasswd_LDFLAGS = $(LDFLAGS_G) \
	$(LDFLAGS_O) \
	$(LDFLAGS_CRYPT)

#######################################
# Define target and object files
#######################################

rfspasswd_OBJS =  src/rfspasswd.o \
	src/crypt.o \
	src/passwd.o \
	src/list.o \
	src/buffer.o \
	src/signals.o
