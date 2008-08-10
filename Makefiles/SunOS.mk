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

CFLAGS_O      = -DSOLARIS \
                -DO_ASYNC=0 \
                -D_REENTRANT \
                -DNAME_MAX=255 \
                -DEREMOTEIO=ECANCELED
CFLAGS_OPT    = -O3
CFLAGS_DBG    = -g

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE   = `pkg-config --cflags fuse`
LDFLAGS_FUSE  = `pkg-config --libs fuse`

###############################
# Flags for linking
###############################

LDFLAGS_NET   = -lsocket -lnsl
LDFLAGS_PTHR  = -lpthread
LDFLAGS_CRYPT = -lcrypt
