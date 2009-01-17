#######################################
# Build CFLAGS/LDFLAGS according to OS
# specific flags (?FLAGS_O) and global
# flags passed by the caller (?FLAGS_G)
#######################################

rfsd_TARGET = "rfsd"

rfsd_CFLAGS  = -D_FILE_OFFSET_BITS=64 \
               $(CFLAGS_MAIN) \
               $(CFLAGS_OS) \
               $(CFLAGS_OPTS)

rfsd_LDFLAGS = $(LDFLAGS_MAIN) \
               -L. -lrfsd \
               $(LDFLAGS_MAIN) \
               $(LDFLAGS_OS) \
               $(LDFLAGS_NET) \
               $(LDFLAGS_OPTS)

#######################################
# Define target and object files
#######################################

rfsd_OBJS = src/rfsd.o \
            src/signals_server.o \
            src/sug_server.o
