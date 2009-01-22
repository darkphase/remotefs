###########################################
# get OS and load the OS specific makefille
###########################################
# GNU make
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname
include mk/$(OS)$(ALT).mk

# You may put your special flags here
CFLAGS = -g

# if you don't want IPv6 support comment in
IPV6 = -DWITH_IPV6

# for programs which use the library
# librfs_nss.so

TOP_INC = `pwd`/include

# define the object files we have
LIB_OBJ  = src/rfs_nss_client.o

SVR_OBJ  = src/dllist.o \
           src/rfs_nss_server.o \
           src/rfs_nss_control.o

CTRL_OBJ = src/rfs_nss_ctrl.o \
           src/rfs_nss_control.o

LIBRFS_OBJ = src/rfs_nss_control.o


all: server client

client: $(CLIENT)
server: $(SERVER)

$(NSSMODULE): $(LIB_OBJ)
	@echo link $@
	@$(CC) -o $(NSSMODULE) $(LIB_OBJ) $(LDFLAGS_SHARED) $(LDFLAGS) $(CFLAGS_GLOB) $(CFLAGS)

rfs_nss: $(SVR_OBJ)
	@echo link $@
	@$(CC) -o rfs_nss $(SVR_OBJ) $(LDFLAGS_NET) $(CFLAGS_GLOB) $(CFLAGS)

rfs_nss_get: src/rfs_nss_get.o $(LIBRFS_OBJ)
	@echo link $@
	@$(CC) -o $@ src/rfs_nss_get.o $(LIBRFS_OBJ) $(CFLAGS_GLOB) $(LDFLAGS_NET) $(CFLAGS)

rfs_nss_rem: src/rfs_nss_rem.o
	@echo link $@
	@$(CC) -o $@ src/rfs_nss_rem.o $(CFLAGS) $(LDLFLAG_NET) $(LDFLAGS_NET)

# The following is not required normally
other: $(OTHER)

rfs_nss_add: src/rfs_nss_add.o $(LIBNAME)
	@echo link $@
	@$(CC) -o rfs_nss_add src/rfs_nss_add.o $(LDLFLAG_DYNLD) $(CFLAGS_GLOB) $(CFLAGS)

rfs_nss_ctrl: $(CTRL_OBJ)
	@echo link $@
	@cc -g -o rfs_nss_ctrl $(CTRL_OBJ) $(LDFLAGS_NET) $(OS_CFLAGS) $(CFLAGS)

$(LIBNAME): $(LIBRFS_OBJ)
	@echo link $@
	@$(CC) -o $(LIBNAME) $(LIBRFS_OBJ) $(LDFLAGS_SHARED) $(LDFLAGS_NET) $(LDFLAGS) $(CFLAGS_GLOB) $(CFLAGS)


# a simple test program
test: rfs_nss_test getpwent getgrent

rfs_nss_test: test/rfs_nss_test.c src/rfs_nss_control.o
	@echo link $@
	@$(CC) -g -o rfs_nss_test test/rfs_nss_test.c src/rfs_nss_control.o $(SVR_LDFLAGS) $(CFLAGS_GLOB) $(CFLAGS)

getpwent: test/getpwent.c
	@echo link $@
	@$(CC) -g -o $@ $<

getgrent: test/getgrent.c
	@echo link $@
	@$(CC) -g -o $@ $<

clean:
	@echo clean
	@if [ "`echo */*.o`" != "*/*.o" ]; then $(RM) */*.o; fi
	@if [ -f $(LIBNAME)    ]; then $(RM) $(LIBNAME);    fi
	@if [ -f $(NSSMODULE)  ]; then $(RM) $(NSSMODULE);  fi
	@if [ -f rfs_nss       ]; then $(RM) rfs_nss;       fi
	@if [ -f rfs_nss_add   ]; then $(RM) rfs_nss_add;   fi
	@if [ -f rfs_nss_ctrl  ]; then $(RM) rfs_nss_ctrl;  fi
	@if [ -f rfs_nss_test  ]; then $(RM) rfs_nss_test;  fi
	@if [ -f rfs_nss_get   ]; then $(RM) rfs_nss_get;   fi
	@if [ -f rfs_nss_rem   ]; then $(RM) rfs_nss_rem;   fi
	@if [ -f getpwent      ]; then $(RM) getpwent;      fi
	@if [ -f getgrent      ]; then $(RM) getgrent;      fi
	@if [ -e rfs_nss.tgz   ]; then $(RM) rfs_nss.tgz;   fi

install:
	@echo install
	@if [ -f rfs_nss      ]; then $(CP) rfs_nss      $(NSS_BIN_DIR); fi
	@if [ -f rfs_nss_get  ]; then $(CP) rfs_nss_get  $(NSS_BIN_DIR); fi
	@if [ -f rfs_nss_rem  ]; then $(CP) rfs_nss_rem  $(NSS_BIN_DIR); fi
	@if [ -f rfs_nss_add  ]; then $(CP) rfs_nss_add  $(NSS_BIN_DIR); fi
	@if [ -f $(NSSMODULE) ]; then $(CP) $(NSSMODULE) $(NSS_LIB_DIR); fi
	@if [ -f $(LIBNAME)   ]; then $(CP) $(NSSMODULE) $(NSS_LIB_DIR); fi

uninstall:
	@echo uninstall
	@if [ -f $(NSS_LIB_DIR)/$(NSSMODULE) ]; then $(NSS_LIB_DIR)/$(NSSMODULE); fi
	@if [ -f $(NSS_LIB_DIR)/$(LIBNAME)   ]; then $(NSS_LIB_DIR)/$(LIBNAME); fi
	@if [ -f $(NSS_BIN_DIR)rfs_nss       ]; then$(CP) rfs_nss     $(NSS_BIN_DIR)/rfs_nss; fi
	@if [ -f $(NSS_BIN_DIR)rfs_nss_get   ]; then$(CP) rfs_nss_get $(NSS_BIN_DIR)/rfs_nss_get; fi
	@if [ -f $(NSS_BIN_DIR)rfs_nss_rem   ]; then$(CP) rfs_nss_rem $(NSS_BIN_DIR)/rfs_nss_rem; fi
	@if [ -f $(NSS_BIN_DIR)rfs_nss_add   ]; then$(CP) rfs_nss_add $(NSS_BIN_DIR)/rfs_nss_rem; fi

tgz: clean
	@echo build tar gz archive
	@mkdir rfs_nss
	@tar -c -T FILES -f - | ( cd rfs_nss; tar xf - )
	@tar czf rfs_nss.tgz rfs_nss
	@rm -fr rfs_nss;

.c.o:
	@echo compile $@
	@cc -c -o $@ $< $(CFLAGS_GLOB) $(IPV6) -I$(TOP_INC) $(CFLAGS) 

# Dependencies

src/rfs_nss_client.o:  src/rfs_nss.h src/rfs_nss_solaris.c src/rfs_nss_freebsd.c
src/rfs_nss_control.o: src/rfs_nss.h
src/rfs_nss_ctrl.o:    src/rfs_nss_control.c src/rfs_nss.h
src/dllist.o:          src/dllist.h
src/rfs_nss_derver.o:  src/rfs_nss.h src/dllist.h src/
src/rfs_nss_rem.o:     src/rfs_nss.h
src/rfs_nss_get.o:     src/rfs_nss.h
src/rfs_nss_add.o:     src/rfs_nss.h
