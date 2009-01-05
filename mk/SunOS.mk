CFLAGS = -DSOLARIS

CC = cc
RM = rm
CP = cp

LDFLAGS = -lresolv

LIBNAME=nss_rfs.so.1

LIB_LDFLAGS = -shared -Wl,-soname,$(LIBNAME)
SVR_LDFLAGS = -lnsl -lsocket -lintl

NSS_LIB_DIR = /usr/lib
NSS_BIN_DIR = /usr/bin
