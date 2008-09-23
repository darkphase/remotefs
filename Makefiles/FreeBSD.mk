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
CFLAGS_G     = -Wall -Werror
CFLAGS_O     = -I./src \
               -I/usr/local/include \
               -DFREEBSD \
               -DEBADE=EINVAL \
               -DEREMOTEIO=ECANCELED \
               -DNAME_MAX=255 
CFLAGS_DBG   = -g
CFLAGS_OPT   = -O3

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  = `pkg-config --cflags fuse`
               -DFUSE_USE_VERSION=25
LDFLAGS_FUSE = `pkg-config --libs fuse`

###############################
# Flags for linking
###############################

LDFLAGS_NET   =
#LDFLAGS_PTHR  = -lpthread
LDFLAGS_CRYPT = -lcrypt

