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
# Target processor for deb packages
################################

TGTARCH = ARCH=armv5tejl

################################
# The executables
################################

MAKE = make
CC = arm-none-linux-gnueabi-gcc
AR = arm-none-linux-gnueabi-ar
RM = rm

################################
# OS / CC specifics flags
################################

CFLAGS_O     = $(DRF)
CFLAGS_DBG   = -g
CFLAGS_OPT   = -O3 -s

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  = `pkg-config --cflags fuse`
LDFLAGS_FUSE = `pkg-config --libs fuse`

###############################
# Flags for linking
###############################

LDFLAGS_NET   =
#LDFLAGS_PTHR  = -lpthread
LDFLAGS_CRYPT = -lcrypt
