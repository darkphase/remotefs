################################
# The executables
################################

MAKE = make
CC = gcc
AR = ar
RM = rm
LN = ln -sf

################################
# OS / CC specifics flags
################################

CFLAGS_OS      = -Wall -Werror -DQNX -I/opt/include
CFLAGS_DEBUG   = -g
CFLAGS_RELEASE = -O3 -s

###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  =
LDFLAGS_FUSE =

###############################
# Flags for linking
###############################

LDFLAGS_NET   = -lsocket
LDFLAGS_SSL   = -L/opt/lib -lssl -lcrypto

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
