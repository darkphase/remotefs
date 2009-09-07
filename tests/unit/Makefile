
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
CPPFLAGS = -I../../src
LDSTATIC = \
           ../../src/exports.o \
           ../../src/id_lookup.o \
           ../../src/passwd.o
LDFLAGS  = $(LDSTATIC) ../../librfs.so -lpthread -lcppunit

default: clean tests

$(FILES):
	@echo "Compiling: $*.cpp"
	$(CPP) -c $*.cpp -o $@ $(CPPFLAGS)

tests: $(FILES)
	@echo "Linking $(TARGET)"
	$(CPP) $(FILES) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)

run:
	@LD_LIBRARY_PATH=../.. ./$(TARGET)

