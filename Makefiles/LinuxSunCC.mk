################################
# The executables
################################

MAKE = make
CC = /opt/sun/sunstudio12/prod/bin/cc
AR = ar
RM = rm
LN = ln -sf

################################
# OS / CC specifics flags
################################
# SunStudio V 11 !
CFLAGS_OS      = -D_GNU_SOURCE \
                 -D"bswap_64(x)=(long long)((long long)htonl(x)<<32)|((long long)htonl((x)>>32))" \
                 -D__FUNCTION__=__func__
CFLAGS_DEBUG   = -g
CFLAGS_RELEASE = -O3


###############################
# Flags needed for Fuse
###############################

CFLAGS_FUSE  = `pkg-config --cflags fuse` -DFUSE_USE_VERSION=26
LDFLAGS_FUSE = -lfuse -lpthread

###############################
# Flags for linking
###############################

LDFLAGS_DEBUG   = -g
LDFLAGS_RELEASE = -s
LDFLAGS_SSL     = -lssl

###############################
# Flags for dymamic libraries
###############################

LDFLAGS_SO   = -shared -Wl,-soname,$(@)
SO_EXT       = so
SO_NAME      = $(TARGET).$(SO_EXT).$(VERSION)

###############################
# Optional OS dependent program
###############################

RFS_NSS = rfs_nss
