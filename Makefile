###########################################
# get OS and load the OS specific makefille
###########################################
# GNU make
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname

all: server client

include mk/$(OS)$(ALT).mk

# Path to remotefs main directory
RFS_PATH = ..

# for testing from top directoty remotefs/rfs_nss:trunk
RFS_PATH = ../../trunk

# include options from remotefs opt1 = ipv6, opt5 = igo
include $(RFS_PATH)/Makefiles/options.mk
CFLAGS_GLOB = $(OPT_1) $(OPT_5)


# You may put your special flags here
INCFLAGS   = -I$(RFS_PATH)/src -I.
RFSLDFLAGS = -L$(RFS_PATH) -lrfs

# define the object files we have
LIB_OBJ  = src/rfs_nss_client.o

SVR_OBJ  = src/dllist.o \
           src/rfs_nss_server.o \
           src/rfs_nss_control.o \
           src/rfs_getnames.o

CTRL_OBJ = src/rfs_nss_ctrl.o \
           src/rfs_nss_control.o \
           src/rfs_getnames.o


all: server client

client: $(CLIENT) $(NSSMODULE)
server: $(SERVER)

$(NSSMODULE): $(LIB_OBJ)
	@echo link $@
	@$(CC) -o $(NSSMODULE) $(LIB_OBJ) $(LDFLAGS_SHARED) $(LDFLAGS) $(CFLAGS_GLOB) $(CFLAGS)

rfs_nss: $(SVR_OBJ)
	@echo link $@
	@$(CC) -o rfs_nss $(SVR_OBJ) $(LDFLAGS_NET) $(CFLAGS_GLOB) $(CFLAGS) $(RFSLDFLAGS) $(INCFLAGS)

nosupport:
	@echo Sorry no support for this OS
	@exit 2

# a simple test program
test: rfs_nss_test getpwent getgrent

rfs_nss_test: test/rfs_nss_test.c src/rfs_nss_control.o
	@echo link $@
	@$(CC) -g -o rfs_nss_test test/rfs_nss_test.c src/rfs_nss_control.o  $(LDFLAGS_NET) $(SVR_LDFLAGS) $(CFLAGS_GLOB) $(CFLAGS)

getpwent: test/getpwent.c
	@echo link $@
	@$(CC) -g -o $@ test/getpwent.c

getgrent: test/getgrent.c
	@echo link $@
	@$(CC) -g -o $@ test/getgrent.c

clean:
	@echo clean
	@if [ "`echo */*.o`" != "*/*.o" ]; then $(RM) */*.o; fi
	@if [ -f $(NSSMODULE)  ]; then $(RM) $(NSSMODULE);   fi
	@if [ -f rfs_nss       ]; then $(RM) rfs_nss;        fi
	@if [ -f getpwent      ]; then $(RM) getpwent;       fi
	@if [ -f getgrent      ]; then $(RM) getgrent;       fi
	@if [ -f rfs_nss_test  ]; then $(RM) rfs_nss_test;   fi
	@if [ -e rfs_nss.tgz   ]; then $(RM) rfs_nss.tgz;    fi

install:
	@echo install
	@if [ -f rfs_nss      ]; then $(CP) rfs_nss      $(NSS_BIN_DIR); fi
	@if [ -f $(NSSMODULE) ]; then $(CP) $(NSSMODULE) $(NSS_LIB_DIR); fi

uninstall:
	@echo uninstall
	@if [ -f $(NSS_LIB_DIR)/$(NSSMODULE) ]; then $(NSS_LIB_DIR)/$(NSSMODULE); fi
	@if [ -f $(NSS_BIN_DIR)/rfs_nss      ]; then $(CP) rfs_nss     $(NSS_BIN_DIR)/rfs_nss; fi

tgz: clean
	@echo build tar gz archive
	@mkdir rfs_nss
	@tar -c -T FILES -f - | ( cd rfs_nss; tar xf - )
	@tar czf rfs_nss.tgz rfs_nss
	@rm -fr rfs_nss;

.c.o:
	@echo compile $@
	@$(CC) -g -c -o $@ $< $(CFLAGS_GLOB) $(IPV6) $(CFLAGS) $(INCFLAGS) $(OS_CFLAGS)

# Dependencies

src/rfs_nss_client.o:  src/rfs_nss.h src/rfs_nss_solaris.c src/rfs_nss_freebsd.c
src/rfs_nss_control.o: src/rfs_nss.h
src/rfs_nss_server.o:  src/rfs_nss.h src/dllist.h src/rfs_getnames.h
src/rfs_getnames.o:    src/rfs_nss.h
src/dllist.o:          src/dllist.h

# external dependencies to for remotefs
src/rfs_getnames.o:    $(RFS_PATH)/src/nss_client.h 
src/rfs_getnames.o:    $(RFS_PATH)/src/list.h 

