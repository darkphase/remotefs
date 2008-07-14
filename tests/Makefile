include ../Makefiles/$(shell uname).mk

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

default: all
all: $(LIB_OBJS) link_lib $(OBJS) link_tests

run: clean all
	@./$(TARGET)

link_tests:
	@echo "Linking $(TARGET)"
	@$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

link_lib:
	@echo "Creating library $(LIB_TARGET)"
	@$(AR) rcs $(LIB_TARGET) $(LIB_OBJS)

%.o : %.cpp
	@echo "Compiling $<"
	@$(CC) -c -Wall $< -o $@ 

%.o : %.c
	@echo "Compiling $<"
	@$(CC) -c -Wall $< -o $@ 

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJS)
	@rm -f $(LIB_TARGET)
	@rm -f $(LIB_OBJS)
