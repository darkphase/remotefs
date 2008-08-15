#######################################
# Build CFLAGS/LDFLAGS according to OS
# CC specific flags 
#######################################

rfspasswd_CFLAGS  = $(CFLAGS_G) \
                    $(CFLAGS_O)

rfspasswd_LDFLAGS = $(LDFLAGS_G) \
                    $(LDFLAGS_O) \
                    $(LDFLAGS_CRYPT)

#######################################
# Define target and object files
#######################################

rfspasswd_OBJS =  src/rfspasswd.o

# the following object are declared
# whithin rfs.mk as com1_OBJS and
# are used for rfs rfsd and rfspasswd
#                  src/crypt.o \
#                  src/passwd.o \
#                  src/list.o \
#                  src/buffer.o \
#                  src/alloc.o \
#                  src/signals.o
