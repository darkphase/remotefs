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

CFLAGS_OS      = -Wall -Werror -m64 -I/usr/include
CFLAGS_DEBUG   = -g -fPIC
CFLAGS_RELEASE = -O2

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  = `pkg-config --cflags fuse` -DFUSE_USE_VERSION=26
LDFLAGS_FUSE = `pkg-config --libs fuse`

###############################
# Flags for linking
###############################

LDFLAGS_DEBUG   = -g -m64 -L/usr/lib/
LDFLAGS_RELEASE = -s -O2 -m64 -L/usr/lib/
LDFLAGS_SSL     = -lssl

###############################
# Flags for dymamic libraries
###############################

CFLAGS_SO    = -fPIC
LDFLAGS_SO   = -shared -Wl,-soname,$(SO_NAME) -fPIC
SO_EXT       = so
SO_NAME      = $(TARGET).$(SO_EXT).$(VERSION)

###############################
# Optional OS dependent program
###############################

RFS_NSS = rfs_nss

