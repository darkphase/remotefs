
TARGET = rfs
CFLAGS += -DFUSE_USE_VERSION=25 `pkg-config --cflags fuse`
LDFLAGS += -lcrypt -lpthread `pkg-config --libs fuse`

OBJS = src/rfs.o \
src/operations.o \
src/buffer.o \
src/command.o \
src/sendrecv.o \
src/signals.o \
src/signals_client.o \
src/alloc.o \
src/attr_cache.o \
src/list.o \
src/passwd.o \
src/crypt.o \
src/keep_alive_client.o \
src/write_cache.o \
src/read_cache.o

include Makefiles/base.mk

default: all
