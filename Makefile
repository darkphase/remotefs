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


# define the object files we have
LIB_OBJ  = src/rfs_nss_client.o

SVR_OBJ  = src/dllist.o \
           src/rfs_nss_server.o \
           src/rfs_nss_control.o

CTRL_OBJ = src/rfs_nss_ctrl.o \
           src/rfs_nss_control.o

LIBRFS_OBJ = src/rfs_nss_control.o


all: $(LIBNAME) rfs_nss rfs_nss_get rfs_nss_rem

client: rfs_nss rfs_nss_get $(LIBNAME)
server: rfs_nss_rem

$(LIBNAME): $(LIB_OBJ)
	@echo link $@
	@$(CC) -o $(LIBNAME) $(LIB_OBJ) $(LIB_LDFLAGS) $(LDFLAGS) $(OS_CFLAGS) $(CFLAGS)

rfs_nss: $(SVR_OBJ)
	@echo link $@
	@$(CC) -o rfs_nss $(SVR_OBJ) $(SVR_LDFLAGS) $(OS_CFLAGS) $(CFLAGS)

rfs_nss_get: src/rfs_nss_get.o $(LIBRFS_OBJ)
	@echo link $@
	@$(CC) -o $@ src/rfs_nss_get.o $(LIBRFS_OBJ) $(CFLAGS) $(OS_CFLAGS) $(LDFLAGS) $(SVR_LDFLAGS)

rfs_nss_rem: src/rfs_nss_rem.o
	@echo link $@
	@$(CC) -o $@ src/rfs_nss_rem.o $(CFLAGS) $(LDFLAGS) $(SVR_LDFLAGS)

# The following is not required normally
other: librfs_nss.so rfs_nss_add rfs_nss_ctrl

rfs_nss_add: src/rfs_nss_add.o $(LIBRFS_OBJ)
	@echo link $@
	@$(CC) -o rfs_nss_add src/rfs_nss_add.o $(LDLFLAG) $(OS_CFLAGS) $(CFLAGS)

rfs_nss_ctrl: $(CTRL_OBJ)
	@echo link $@
	@cc -g -o rfs_nss_ctrl $(CTRL_OBJ) $(SVR_LDFLAGS) $(OS_CFLAGS) $(CFLAGS)

librfs_nss.so: $(LIBRFS_OBJ)
	@echo link $@
	@$(CC) -o librfs_nss.so $(LIBRFS_OBJ) $(LIB_LDFLAGS) $(LDFLAGS) $(OS_CFLAGS) $(CFLAGS)


# a simple test program
test: rfs_nss_test

rfs_nss_test: test/rfs_nss_test.c src/rfs_nss_control.o
	@echo link $@
	@$(CC) -g -o rfs_nss_test test/rfs_nss_test.c src/rfs_nss_control.o $(SVR_LDFLAGS) $(OS_CFLAGS) $(CFLAGS)

clean:
	@echo clean
	@if [ "`echo */*.o`" != "*/*.o" ]; then $(RM) */*.o; fi
	@if [ -f $(LIBNAME)    ]; then $(RM) $(LIBNAME);    fi
	@if [ -f librfs_nss.so ]; then $(RM) librfs_nss.so; fi
	@if [ -f rfs_nss       ]; then $(RM) rfs_nss;       fi
	@if [ -f rfs_nss_add   ]; then $(RM) rfs_nss_add;   fi
	@if [ -f rfs_nss_ctrl  ]; then $(RM) rfs_nss_ctrl;  fi
	@if [ -f rfs_nss_test  ]; then $(RM) rfs_nss_test;  fi
	@if [ -f rfs_nss_get   ]; then $(RM) rfs_nss_get;   fi
	@if [ -f rfs_nss_rem   ]; then $(RM) rfs_nss_rem;   fi

install:
	@echo install
	@if [ -f rfs_nss     ]; then $(CP) rfs_nss     $(NSS_BIN_DIR); fi
	@if [ -f rfs_nss_get ]; then $(CP) rfs_nss_get $(NSS_BIN_DIR); fi
	@if [ -f rfs_nss_rem ]; then $(CP) rfs_nss_rem $(NSS_BIN_DIR); fi
	@if [ -f $(LIBNAME)  ]; then $(CP) $(LIBNAME) $(NSS_LIB_DIR);  fi

uninstall:
	@echo uninstall
	@if [ -f $(NSS_LIB_DIR)/$(LIBNAME) ]; then $(NSS_LIB_DIR)/$(LIBNAME); fi
	@if [ -f $(NSS_BIN_DIR)rfs_nss     ]; then$(CP) rfs_nss     $(NSS_BIN_DIR)/rfs_nss; fi
	@if [ -f $(NSS_BIN_DIR)rfs_nss_get ]; then$(CP) rfs_nss_get $(NSS_BIN_DIR)/rfs_nss_get; fi
	@if [ -f $(NSS_BIN_DIR)rfs_nss_rem ]; then$(CP) rfs_nss_rem $(NSS_BIN_DIR)/rfs_nss_rem; fi

tgz: clean
	@echo build tar gz archive
	@DIR=`pwd`; DIR=`basename $$DIR`; cd ..; tar czf $$DIR.tgz $$DIR

.c.o:
	@echo compile $@
	@cc -c -o $@ $< $(OS_CFLAGS) $(CFLAGS) $(IPV6)

# Dependencies

src/rfs_nss_client.o:  src/rfs_nss.h src/rfs_nss_solaris.c src/rfs_nss_freebsd.c
src/rfs_nss_control.o: src/rfs_nss.h
src/rfs_nss_ctrl.o:    src/rfs_nss_control.c src/rfs_nss.h
src/dllist.o:          src/dllist.h
src/rfs_nss_derver.o:  src/rfs_nss.h src/dllist.h src/
src/rfs_nss_rem.o:     src/rfs_nss.h
src/rfs_nss_get.o:     src/rfs_nss.h
src/rfs_nss_add.o:     src/rfs_nss.h
