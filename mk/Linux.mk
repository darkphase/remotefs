OS_CFLAGS = -Wall -Werror -O2

CC = cc
RM = rm
CP = cp


LIBNAME=libnss_rfs.so.2
LIB_LDFLAGS= -shared -Wl,-soname,$(LIBNAME)
LDLFLAG = -ldl

NSS_LIB_DIR = /lib
NSS_BIN_DIR = /usr/bin
