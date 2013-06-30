
OS=$(shell uname)
OS:sh=uname

include build/Makefiles/$(OS)$(ALT).mk
include build/Makefiles/options.mk
include build/Makefiles/version.mk
include build/Makefiles/variable/custom.mk
include build/Makefiles/variable/verbosity.mk
include build/Makefiles/rfsd-defs.mk
include build/Makefiles/librfs-defs.mk

CFLAGS      = $(CFLAGS_RELEASE) $(CFLAGS_OS) $(CFLAGS_OPTS) $(CFLAGS_FUSE) -D_FILE_OFFSET_BITS=64 -DRFS_DEBUG -g
LDFLAGS     = 
RFSLIBFILES = $(librfs_OBJS)
RFSDLIBFILES = $(rfsd_OBJS)
CC          = gcc

TARGET      = units

STUBFILES   = tests/stub/client.o \
              tests/stub/framework.o \
              tests/stub/server.o \
              tests/stub/sockets.o
              
FILES       = tests/framework_test.o \
              tests/op_badmsg.o \
              tests/op_conn_aborted.o \
              tests/op_open.o \
              \
              tests/suite.o \
              tests/attr_cache.o \
              tests/auth.o \
              tests/buffer.o \
              tests/changelog.o \
              tests/cleanup.o \
              tests/crypt.o \
              tests/error.o \
              tests/exports.o \
              tests/id_lookup.o \
              tests/inet.o \
              tests/list.o \
              tests/md5.o \
              tests/names.o \
              tests/path.o \
              tests/passwd.o \
              tests/resolve.o \
              tests/resume.o \
              tests/sendrecv.o \
              tests/sockets.o \
              tests/transform_symlink.o \
              tests/utils.o
              
CPP         = g++
RFSLIB      = librfslib.a
RFSDLIB     = librfsdlib.a
STUBLIB     = libstublib.a
AR          = ar -r

CPPFLAGS    = $(CFLAGS) -ggdb
CPPLDFLAGS  = $(LDFLAGS) -lpthread `pkg-config --libs cppunit` -L. -lstublib -lrfslib -lrfsdlib

rfslib: make_version $(RFSLIBFILES)
	@echo "Creating $(RFSLIB)"
	$(AR) $(RFSLIB) $(RFSLIBFILES) >$(OUTPUT) 2>&1

rfsdlib: make_version $(RFSDLIBFILES)
	@echo "Creating $(RFSDLIB)"
	$(AR) $(RFSDLIB) $(RFSDLIBFILES) >$(OUTPUT) 2>&1

stublib: $(STUBFILES)
	@echo "Creating $(STUBLIB)"
	$(AR) $(STUBLIB) $(STUBFILES) >$(OUTPUT) 2>&1

$(STUBFILES):
	@echo "Compiling: $*.cpp"
	$(CPP) $(CPPFLAGS) -c $*.cpp -o $@

$(LIBFILES):
	@echo "Compiling: $*.c"
	$(CC) $(CFLAGS) -c $*.c -o $@

$(FILES):
	@echo "Compiling: $*.cpp"
	$(CPP) $(CPPFLAGS) -c $*.cpp -o $@

builddep: dummy
	grep -E '#\s*include[^"]+"[^"]+"' $(SCANDIR)*.cpp | sed -r -e 's/\.cpp/.o/' -e 's/#\s*include[^"]+"([^"]+)".*/$(SCANDIR)\1/' >> build/depends.mk
	ls $(SCANDIR)*.cpp | sed -e 's/\([^\.]*\)/\1.o:\1/' >> build/depends.mk

depends: dummy
	echo -n "" > build/depends.mk
	SCANDIR="tests\/" $(MAKE) -f base.mk builddep
	SCANDIR="tests\/stub\/" $(MAKE) -f base.mk builddep

compile: dummy 
	$(MAKE) $(SILENT) -f base.mk $(FILES)

build: clean_link stublib
	LIBFILES="$(librfs_OBJS)" $(MAKE) $(SILENT) -f base.mk rfslib
	LIBFILES="$(rfsd_OBJS)" $(MAKE) $(SILENT) -f base.mk rfsdlib
	$(MAKE) $(SILENT) -f base.mk compile
	@echo "Linking $(TARGET)"
	$(CPP) $(FILES) $(CPPLDFLAGS) -o $(TARGET)

clean_link: dummy
	rm -f $(TARGET)

clean: clean_link
	rm -f tests/*.d tests/*.o
	rm -f tests/stub/*.d tests/stub/*.o
	rm -f $(RFSLIB) $(RFSDLIB)
	rm -f $(STUBLIB)
	$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean

run:
	./$(TARGET) 2>$(OUTPUT)

dummy:

include build/depends.mk
include build/Makefiles/depends.mk
