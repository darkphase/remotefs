################################
# The executables
################################

MAKE = make
CC = cc
AR = ar
RM = rm

###############################
# wrong behaviour for dmake
################################
SUB=$(@F:%.o=%.c)

################################
# OS / CC specifics flags
################################
CFLAGS_O     = -I./src -DFREEBSD -DEBADE=EINVAL -DEREMOTEIO=ECANCELED -DNAME_MAX=255 
CFLAGS_DBG   = -g
CFLAGS_OPT   = -O3

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  = -I/usr/include/fuse -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=25
LDFLAGS_FUSE = -lfuse

###############################
# Flags for linking
###############################

LDFLAGS_NET   =
#LDFLAGS_PTHR  = -lpthread
LDFLAGS_CRYPT = -lcrypt

