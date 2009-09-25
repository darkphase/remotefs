
TARGET   = tests
FILES    = suite.o \
           test_buffer.o \
		   test_error.o \
		   test_exports.o \
		   test_id_lookup.o \
		   test_inet.o \
		   test_list.o \
		   test_path.o \
		   test_passwd.o \
		   test_sendrecv.o \
           test_utils.o
CPP      = g++
MAKEDEPEND = gcc -M
CPPFLAGS = -I../../src
LDSTATIC = \
           ../../src/exports.o \
           ../../src/id_lookup.o \
           ../../src/passwd.o
LDFLAGS  = $(LDSTATIC) ../../librfs.so -lpthread -lcppunit
DEPENDS  = dependencies

default: tests

$(FILES):
	$(MAKEDEPEND) $(CPPFLAGS) -o $*.d $*.cpp
	@echo "Compiling: $*.cpp"
	$(CPP) -c $*.cpp -o $@ $(CPPFLAGS)

tests: clean_link $(FILES)
	@echo "Linking $(TARGET)"
	$(CPP) $(FILES) $(LDFLAGS) -o $(TARGET)

clean_link:
	rm -f $(TARGET)

clean: clean_link
	rm -f *.d *.o

run:
	@LD_LIBRARY_PATH=../.. ./$(TARGET)

-include *.d

