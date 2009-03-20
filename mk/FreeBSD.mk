###########################################
# The executables
###########################################

CC = cc
RM = rm
CP = cp

###########################################
# CFLAGS for debuging or optimization
# CFLAGS for OS specific includes, ...
###########################################

CFLAGS_OPT  = -O2
OS_CFLAGS   = -DFREEBSD
# we use this within the main Makefile
CFLAGS_GLOB = $(OS_CFLAGS) $(CFLAGS_OPT)


###########################################
# LDFLAGS 
###########################################

LDFLAGS_SHARED = -shared -Wl,-soname,$(LIBNAME)
LDFLAGS_NET    = 
LDLFLAG_DYNLD  = 

#LDFLAGS = -lresolv

###########################################
# NAME for nss module, OS specific
###########################################

NSSMODULE = nss_rfs.so.1

###########################################
# Where to install, OS specific
###########################################

NSS_LIB_DIR = /usr/local/lib
NSS_BIN_DIR = /usr/bin

##########################################
# What we can compile
##########################################

CLIENT  = $(NSSMODULE) rfs_nss rfs_nss_get
SERVER  = 
OTHER   = 
