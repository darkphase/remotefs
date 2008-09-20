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

CFLAGS_O     =
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
