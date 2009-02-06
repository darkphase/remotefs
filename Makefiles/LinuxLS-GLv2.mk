#########################################
#
# For cross-compiling under Linux for
# the buffalo Link Station LS-GL V2
# with arm processor.
#
# The path for searchinh the cross compile
# binaries must be set correctly so we
# can found them.
##########################################

################################
# Target processor for deb/rpm packages
################################

ARCH = arm

################################
# The executables
################################

MAKE = make
CC = arm-none-linux-gnueabi-gcc
AR = arm-none-linux-gnueabi-ar
RM = rm
LN = ln -sf

################################
# OS / CC specifics flags
################################

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

RFS_NSS = 
