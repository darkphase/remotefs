CFLAGS = -Wall -Werror -DFREEBSD

CC = cc
RM = rm
CP = cp

LIBNAME=nss_rfs.so.1
LIB_LDFLAGS= -shared -Wl,-soname,$(LIBNAME)

NSS_LIB_DIR = /usr/local/lib
NSS_BIN_DIR = /usr/bin
