#######################################
# Build CFLAGS/LDFLAGS according to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

rfsd_CFLAGS  = $(CFLAGS_G) \
               $(CFLAGS_O) 

rfsd_LDFLAGS = $(LDFLAGS_G) \
               $(LDFLAGS_O) \
               $(LDFLAGS_NET) \
               $(LDFLAGS_PTHR) \
               $(LDFLAGS_CRYPT)

#######################################
# Define target and object files
#######################################

rfsd_OBJS =  src/rfsd.o \
             src/server_handlers.o \
             src/exports.o \
             src/signals_server.o \
             src/keep_alive_server.o

# the following object are declared
# whithin rfs.mk as com1_OBJS and
# are used for rfs rfsd ans rfspasswd
# com1_OBJS
#             src/crypt.o \
#             src/passwd.o \
#             src/list.o \
#             src/buffer.o \
#             src/alloc.o \
#             src/signals.o 

# the following object are declared
# whithin rfs.mk as com1_OBJS and
# are used for rfs and rfsd
# com2_OBJS
#             src/sendrecv.o \
#             src/command.o \
#             src/path.o

