################################
# The executables
################################

MAKE = make
CC = /opt/sun/sunstudio12/prod/bin/cc
AR = ar
RM = rm

################################
# OS / CC specifics flags
################################
# SunStudio V 11 !
CFLAGS_O     = -D"bswap_64(x)=(long long)((long long)htonl(x)<<32)|((long long)htonl((x)>>32))"
CFLAGS_DBG   = -g
CFLAGS_OPT   = -O3


###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  = `pkg-config --cflags fuse`
LDFLAGS_FUSE = -lfuse -lpthread

###############################
# Flags for linking
###############################

LDFLAGS_NET   =
#LDFLAGS_PTHR  = -lpthread
LDFLAGS_CRYPT = -lcrypt
