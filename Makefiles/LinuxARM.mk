################################
# The executables
################################

TOOLCHAIN_ROOT = toolchains/toolchain-arm_gcc4.1.2
ARCH = powerpc
MAKE = make
CC = "$(TOOLCHAIN_ROOT)/gcc-4.1.2-final/gcc/gcc-cross"
AR = "$(TOOLCHAIN_ROOT)/binutils-2.17/binutils/ar"
RM = rm
LN = ln -sf

################################
# OS / CC specifics flags
################################

CFLAGS_OS      = -Wall -Werror
CFLAGS_DEBUG   = -g
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
LDFLAGS_SO   = -shared -Wl,-soname,$(SO_NAME)
SO_EXT       = so
SO_NAME      = $(TARGET).$(SO_EXT).$(VERSION)

###############################
# Optional OS dependent program
###############################

RFS_NSS = rfs_nss
