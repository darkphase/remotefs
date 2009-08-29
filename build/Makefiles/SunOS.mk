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

CFLAGS_OS      = -DSOLARIS \
                 -D_REENTRANT \
                 -D_XPG5 \
                 -D__FUNCTION__=__func__ \
                 
CFLAGS_RELEASE = -O3
CFLAGS_DEBUG   = -g

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

LDFLAGS_SO   = -shared
SO_EXT       = so
SO_NAME      = $(TARGET).$(SO_EXT).$(VERSION)

###############################
# Optional OS dependent program
###############################

SO_NAME_NSS    = nss_rfs.so.1
LDFLAGS_SO_NSS = -shared -Wl,-soname,$(SO_NAME_NSS) -fPIC
CFLAGS_SO_NSS  = $(CFLAGS_SO)
