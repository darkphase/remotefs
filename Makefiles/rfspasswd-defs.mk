#######################################
# Build CFLAGS/LDFLAGS according to OS
# CC specific flags 
#######################################

rfspasswd_TARGET = "rfspasswd"

rfspasswd_CFLAGS  = $(CFLAGS_MAIN) \
                    $(CFLAGS_OS) \
                    $(CFLAGS_OPTS)

rfspasswd_LDFLAGS = $(LDFLAGS_MAIN) \
                    $(LDFLAGS_OS) \
                    $(LDFLAGS_NET) \
                    $(LDFLAGS_OPTS)


OS_LIBS = $(rfspasswd_LDFLAGS)

#######################################
# Define target and object files
#######################################

rfspasswd_OBJS   = src/buffer.o \
                   src/crypt.o \
                   src/list.o \
                   src/md5crypt/crypt_md5.o \
                   src/md5crypt/md5.o \
                   src/passwd.o \
                   src/signals.o \
                   \
                   src/rfspasswd.o
