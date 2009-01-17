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
                 -D_GNU_SOURCE
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
