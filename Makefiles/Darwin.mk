################################
# The executables
################################

MAKE = make
CC = cc
AR = ar
RM = rm
LN = ln -sf

################################
# OS / CC specifics flags
################################

CFLAGS_OS      = -Wall -DDARWIN
CFLAGS_DDEBUG  = -g
CFLAGS_RELEASE = -O3 -s

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  = -D_FILE_OFFSET_BITS=64 \
               -I/usr/local/include \
               -I/usr/local/include/fuse \
               -I/sw/include \
               -I/sw/include/fuse \
               -DFUSE_USE_VERSION=26

###############################
# Flags for linking
###############################

LDFLAGS_FUSE = -lfuse
LDFLAGS_SSL  = -lssl -lcrypto

###############################
# Flags for dymamic libraries
###############################

LDFLAGS_SO   = -dynamiclib -Wl,-single_module,-flat_namespace,-undefined,dynamic_lookup
SO_EXT       = dylib
SO_NAME      = $(TARGET).$(VERSION).$(SO_EXT)

###############################
# Optional OS dependent program
###############################

RFS_NSS = 
