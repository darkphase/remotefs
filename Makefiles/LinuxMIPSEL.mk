################################
# The executables
################################

TOOLCHAIN_ROOT = toolchains/toolchain-mipsel_gcc3.4.6
ARCH = mipsel
MAKE = make
CC = "$(TOOLCHAIN_ROOT)/bin/mipsel-linux-uclibc-gcc"
AR = "$(TOOLCHAIN_ROOT)/bin/mipsel-linux-uclibc-ar"
RM = rm
LN = ln -sf

################################
# OS / CC specifics flags
################################

CFLAGS_OS      = -Wall -Werror
CFLAGS_DEBUG   = -g
CFLAGS_RELEASE = -O3

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  = `pkg-config --cflags fuse` -DFUSE_USE_VERSION=26
LDFLAGS_FUSE = `pkg-config --libs fuse`

###############################
# Flags for linking
###############################

LDFLAGS_DEBUG   = -g
LDFLAGS_RELEASE = -s -O3
LDFLAGS_SSL     = -lssl

###############################
# Flags for dymamic libraries
###############################

CFLAGS_SO    = -fPIC
LDFLAGS_SO   = -shared -Wl,-soname,$(SO_NAME)
SO_EXT       = so
SO_NAME      = $(TARGET).$(SO_EXT).$(VERSION)

###############################
# Optional OS dependent program
###############################

RFS_NSS = rfs_nss
