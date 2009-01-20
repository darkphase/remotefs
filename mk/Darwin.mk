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
OS_CFLAGS   = -Wall -Werror -DDARWIN
# we use this within the main Makefile
CFLAGS_GLOB = $(OS_CFLAGS) $(CFLAGS_OPT)

###########################################
# LDFLAGS 
###########################################

LDFLAGS_SHARED = -dynamiclib -Wl,-single_module,-flat_namespace,-undefined,dynamic_lookup
LDFLAGS_NET    = 
LDLFLAG_DYNLD  = -ldl

###########################################
# NAME for nss module, OS specific
###########################################

NSSMODULE = 

###########################################
# NAME for librfs_nss
###########################################

LIBNAME = librfs_nss.dly

###########################################
# Where to install, OS specific
###########################################

NSS_LIB_DIR = /lib
NSS_BIN_DIR = /usr/bin

##########################################
# What we can compile
##########################################

CLIENT  = 
SERVER  = rfs_nss_rem
OTHER   = 
