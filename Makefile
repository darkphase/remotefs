CC = gcc
CFLAGS = -DRFS_DEBUG -DFUSE_USE_VERSION=25 `pkg-config --cflags fuse` -g
LDFLAGS = `pkg-config --libs fuse` -lcrypt
TARGET = rfs

OBJS = src/rfs.o \
src/operations.o \
src/buffer.o \
src/command.o \
src/sendrecv.o \
src/signals.o \
src/signals_client.o \
src/alloc.o \
src/attr_cache.o \
src/passwd.o \
src/crypt.o \
src/list.o \
src/keep_alive_client.o

default: $(OBJS) link_client
	ln -sf bin/$(TARGET) $(TARGET)
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(MAKE) -f Makefile.rfsd
	CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(MAKE) -f Makefile.passwd

link_client:
	$(CC) $(OBJS) $(LDFLAGS) -o bin/$(TARGET)
	
%.o : %.c
	$(CC) -c $(CFLAGS) -Wall $< -o $@

clean:
	rm -f src/*.o
	rm -f bin/$(TARGET) $(TARGET)
	$(MAKE) -f Makefile.rfsd clean
	$(MAKE) -f Makefile.passwd clean
	
