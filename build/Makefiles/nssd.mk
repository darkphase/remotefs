
include build/Makefiles/base.mk
include build/Makefiles/nssd-defs.mk

$(nssd_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(nssd_CFLAGS)

build: $(nssd_OBJS)
	@echo Link $(nssd_TARGET)
	$(CC) -o $(nssd_TARGET) $(nssd_OBJS) $(nssd_LDFLAGS)

install_nssd:
	if [ -f $(nssd_TARGET) ]; then \
	    mkdir -p $(INSTALL_DIR)/bin; \
	    cp $(nssd_TARGET) $(INSTALL_DIR)/bin; \
	fi
	
uninstall_nssd:
	if [ -f $(INSTALL_DIR)/bin/$(nssd_TARGET) ]; then \
	    rm -f $(INSTALL_DIR)/bin/$(nssd_TARGET); \
	fi

flags:
	@echo Build rfs_nss
	@echo CFLAGS = $(nssd_CFLAGS)
	@echo LDFLAGS = $(nssd_LDFLAGS)
	@echo
