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

CFLAGS_OS      = -Wall -Werror -DLINUX
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

LDFLAGS_DEBUG   = -g
LDFLAGS_RELEASE = -s -O2
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

SO_NAME_NSS    = libnss_rfs.so.2
LDFLAGS_SO_NSS = -shared -Wl,-soname,$(SO_NAME_NSS) -fPIC
CFLAGS_SO_NSS  = $(CFLAGS_SO)

