################################
# The executables
################################

MAKE = make
CC = gcc
AR = ar
RM = rm

################################
# OS / CC specifics flags
################################

CFLAGS_O      = -DSOLARIS \
                -D_REENTRANT \
                 $(DRF)

CFLAGS_OPT    = -O3

CFLAGS_DBG    = -g -Wall

LDFLAGS_O     = $(DRF)

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE   = `pkg-config --cflags fuse`
LDFLAGS_FUSE  = `pkg-config --libs fuse`

###############################
# Flags for linking
###############################

LDFLAGS_NET   = -lsocket -lnsl
#LDFLAGS_PTHR  = -lpthread
LDFLAGS_CRYPT = -lcrypt
