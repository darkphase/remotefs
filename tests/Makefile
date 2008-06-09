CC=gcc
AR=ar

TARGET=tests
LIB_TARGET=librfs.a

LDFLAGS=-lcppunit -L. -lrfs

OBJS=tests.o \
test_read_cache.o \
test_write_cache.o

LIB_OBJS= ../src/read_cache.o \
../src/write_cache.o \
../src/list.o \
../src/buffer.o \
../src/alloc.o

default: $(OBJS) $(LIB_OBJS) link_lib link_tests

link_tests:
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

link_lib:
	$(AR) rcs $(LIB_TARGET) $(LIB_OBJS)

%.o : %.cpp
	$(CC) -c -Wall $< -o $@ 

%.o : %.c
	$(CC) -c -Wall $< -o $@ 

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)
	rm -f $(LIB_TARGET)
	rm -f $(LIB_OBJS)

