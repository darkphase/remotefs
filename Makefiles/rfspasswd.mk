
TARGET = rfspasswd
LDFLAGS += -lcrypt

OBJS = src/rfspasswd.o \
src/passwd.o \
src/crypt.o \
src/list.o \
src/buffer.o \
src/alloc.o \
src/signals.o

include Makefiles/base.mk

default: all
