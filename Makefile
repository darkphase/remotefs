# GNU make
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname
include mk/$(OS)$(ALT).mk


LIB_OBJ = rfs_nss_client.o

OBJ = dllist.o \
      rfs_nss_server.o \
      rfs_nss_control.o

all: $(LIBNAME) rfs_nss t rfs_nss_ctrl

$(LIBNAME): $(LIB_OBJ)
	$(CC) -o $(LIBNAME) $(LIB_OBJ) $(LIB_LDFLAGS) $(LDFLAGS)

clean:
	@-$(RM) *.o *.so* rfs_nss rfs_nss_ctrl t 2>/dev/null

rfs_nss: $(OBJ)
	$(CC) -o rfs_nss $(OBJ)  $(SVR_LDFLAGS)

# a simple test program
t: t.c $(LIB_OBJ)
	$(CC) -g -o t t.c $(SVR_LDFLAGS)

rfs_nss_ctrl: rfs_nss_ctrl.o rfs_nss_control.o
	cc -g -o rfs_nss_ctrl rfs_nss_ctrl.o rfs_nss_control.o $(SVR_LDFLAGS)

install: $(LIBNAME) rfs_nss
	@echo install $(LIBNAME) rfs_nss
	@$(CP) $(LIBNAME) $(NSS_LIB_DIR)
	@$(CP) rfs_nss    $(NSS_BIN_DIR)

uninstall:
	@echo uninstall $(LIBNAME) rfs_nss
	@$(RM) $(NSS_LIB_DIR)/$(LIBNAME)
	@$(RM) $(NSS_BIN_DIR)/rfs_nss

tgz: clean
	@DIR=`pwd`; DIR=`basename $$DIR`; cd ..; tar czf $$DIR.tgz $$DIR

# Dependencies
rfs_nss_client.o: rfs_nss.h rfs_nss_solaris.c rfs_nss_freebsd.c
rfs_nss_control.o: rfs_nss.h
rfs_nss_ctrl.o: rfs_nss_control.c rfs_nss.h
dllist.o: dllist.h
rfs_nss_server.o: rfs_nss.h dllist.h
