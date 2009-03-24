################################
# The executables
################################

MAKE = make
CC = cc
AR = ar
RM = rm
LN = ln -sf

###############################
# wrong behaviour for dmake
################################
SUB=$(@F:%.o=%.c)

################################
# OS / CC specifics flags
################################
CFLAGS_OS      = -Wall -Werror \
                 -I./src \
                 -I/usr/local/include \
                 -DFREEBSD
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

LDFLAGS_SSL = -lssl

###############################
# Flags for dymamic libraries
###############################

LDFLAGS_SO   = -shared -Wl,-soname,$(@)
SO_EXT       = so
SO_NAME      = $(TARGET).$(SO_EXT).$(VERSION)

###############################
# Optional OS dependent program
###############################

SO_NAME_NSS    = nss_rfs.so.1
LDFLAGS_SO_NSS = -shared -Wl,-soname,$(SO_NAME_NSS) -fPIC
CFLAGS_SO_NSS  = $(CFLAGS_SO)

