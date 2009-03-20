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
OS_CFLAGS   = -DQNX -Wall -Werror
# we use this within the main Makefile
CFLAGS_GLOB = $(OS_CFLAGS) $(CFLAGS_OPT)

###########################################
# LDFLAGS 
###########################################

LDFLAGS_NET    = -lsocket
LDLFLAG_DYNLD  = -ldl

###########################################
# NAME for nss module, OS specific
###########################################

NSSMODULE = libnss_rfs.so.2

###########################################
# Where to install, OS specific
###########################################

NSS_LIB_DIR = /lib
NSS_BIN_DIR = /usr/bin

##########################################
# What we can compile
##########################################

CLIENT  = nosupport
SERVER  = 
OTHER   = 
