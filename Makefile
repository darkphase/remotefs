# GNU make
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname
include mk/$(OS)$(ALT).mk

# defime the object file we have
LIB_OBJ  = src/rfs_nss_client.o

SVR_OBJ  = src/dllist.o \
           src/rfs_nss_server.o \
           src/rfs_nss_control.o

CTRL_OBJ = src/rfs_nss_ctrl.o \
           src/rfs_nss_control.o


all: $(LIBNAME) rfs_nss rfs_nss_ctrl

$(LIBNAME): $(LIB_OBJ)
	$(CC) -o $(LIBNAME) $(LIB_OBJ) $(LIB_LDFLAGS) $(LDFLAGS)

rfs_nss: $(SVR_OBJ)
	$(CC) -o rfs_nss $(SVR_OBJ) $(SVR_LDFLAGS)

rfs_nss_ctrl: $(CTRL_OBJ)
	cc -g -o rfs_nss_ctrl $(CTRL_OBJ) $(SVR_LDFLAGS)

# a simple test program
rfd_nss_test: test/rfd_nss_test.c $(LIB_OBJ)
	$(CC) -g -o test/rfs_nss_test test/rfd_nss_test.c $(SVR_LDFLAGS)

clean:
	@if [ "`echo */*.o`" != "*/*.o" ]; then $(RM) */*.o; fi
	@if [ -f $(LIBNAME)   ]; then $(RM) $(LIBNAME); fi
	@if [ -f rfs_nss      ]; then  $(RM) rfs_nss; fi
	@if [ -f rfs_nss_ctrl ]; then $(RM) rfs_nss_ctrl; fi

install: $(LIBNAME) rfs_nss
	@echo install $(LIBNAME) rfs_nss
	@$(CP) $(LIBNAME)    $(NSS_LIB_DIR)
	@$(CP) rfs_nss       $(NSS_BIN_DIR)
	@$(CP) rfs_nss_ctlr  $(NSS_BIN_DIR)

uninstall:
	@echo uninstall $(LIBNAME) rfs_nss
	@$(RM) $(NSS_LIB_DIR)/$(LIBNAME)
	@$(RM) $(NSS_BIN_DIR)/rfs_nss
	@$(RM) $(NSS_BIN_DIR)/rfs_nss_ctrl

tgz: clean
	@DIR=`pwd`; DIR=`basename $$DIR`; cd ..; tar czf $$DIR.tgz $$DIR

# Dependencies
src/rfs_nss_client.o: src/rfs_nss.h src/rfs_nss_solaris.c src/rfs_nss_freebsd.c
src/rfs_nss_control.o: src/rfs_nss.h
src/rfs_nss_ctrl.o: src/rfs_nss_control.c src/rfs_nss.h
src/dllist.o: src/dllist.h
src/rfs_nss_server.o: src/rfs_nss.h src/dllist.h
