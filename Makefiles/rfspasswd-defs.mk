#######################################
# Build CFLAGS/LDFLAGS according to OS
# CC specific flags 
#######################################

rfspasswd_TARGET = "rfspasswd"

rfspasswd_CFLAGS  = $(CFLAGS_MAIN) \
                    $(CFLAGS_OS) \
                    $(CFLAGS_OPTS)

rfspasswd_LDFLAGS = $(LDFLAGS_MAIN) \
                    -L. -lrfsd \
                    $(LDFLAGS_OS) \
                    $(LDFLAGS_NET) \
                    $(LDFLAGS_OPTS)


OS_LIBS = $(rfspasswd_LDFLAGS)

#######################################
# Define target and object files
#######################################

rfspasswd_OBJS   = src/rfspasswd.o
