################################
# The executables
################################

MAKE = make
CC = cc
AR = ar
RM = rm

################################
# OS / CC specifics flags
################################

CFLAGS_OS      = 
CFLAGS_DEBUG   = -g
CFLAGS_RELEASE = -Os

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
LDFLAGS_NET     = 
LDFLAGS_PTHR    =
LDFLAGS_SSL     =
