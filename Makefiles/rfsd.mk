
TARGET = rfsd
LDFLAGS += -lcrypt

OBJS = src/rfsd.o \
src/sendrecv.o \
src/signals.o \
src/command.o \
src/buffer.o \
src/server_handlers.o \
src/signals_server.o \
src/alloc.o \
src/list.o \
src/exports.o \
src/passwd.o \
src/keep_alive_server.o \
src/crypt.o

include Makefiles/base.mk

default: all
