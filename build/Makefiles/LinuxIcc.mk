################################
# The executables
################################

MAKE = make
CC = icc
AR = ar
RM = rm
LN = ln -sf

################################
# OS / CC specifics flags
################################

CFLAGS_OS      = -Wall \
                 -Werror \
                 -Wno-unused-variable \
                 -Wno-abi \
                 -std=gnu89 \
                 -w1 \
                 -D_GNU_SOURCE \
                 -DLINUX

CFLAGS_DEBUG   = -g
CFLAGS_RELEASE = -O3 -s

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  = `pkg-config --cflags fuse` -DFUSE_USE_VERSION=26
LDFLAGS_FUSE = `pkg-config --libs fuse`

###############################
# Flags for linking
###############################

LDFLAGS_DEBUG   = -g
LDFLAGS_RELEASE = -s
LDFLAGS_SSL     = -lssl
LDFLAGS_ACL     = -lacl -lrt
LDFLAGS_PTHR	= -pthread


###############################
# Flags for dymamic libraries
###############################

LDFLAGS_SO   = -shared -Wl,-soname,$(@)
SO_EXT       = so
SO_NAME      = $(TARGET).$(SO_EXT).$(VERSION)

###############################
# Optional OS dependent program
###############################

SO_NAME_NSS    = libnss_rfs.so.2
LDFLAGS_SO_NSS = -shared -Wl,-soname,$(SO_NAME_NSS) -fPIC
CFLAGS_SO_NSS  = $(CFLAGS_SO)

ALL            = server client nss
