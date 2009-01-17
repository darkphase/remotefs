################################
# The executables
################################

MAKE = make
CC = gcc
AR = ar
RM = rm
LN = ln -sf

################################
# OS / CC specifics flags
################################

CFLAGS_OS      = -DSOLARIS \
                 -D_XPG5 \
                 -D_REENTRANT

CFLAGS_RELEASE = -O3
CFLAGS_DEBUG   = -g -Wall

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE   = -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 -I/usr/include/fuse
LDFLAGS_FUSE  = -lfuse

###############################
# Flags for linking
###############################

LDFLAGS_NET   = -lsocket -lnsl -lsendfile
LDFLAGS_PTHR  = -lpthread
LDFLAGS_SSL   = -lssl -lcrypto

###############################
# Flags for dymamic libraries
###############################

LDFLAGS_SO   = -shared -Wl,-soname,$(@)
SO_EXT       = so
SO_NAME      = $(TARGET).$(SO_EXT).$(VERSION)

###############################
# Optional OS dependent program
###############################

RFS_NSS = rfs_nss
