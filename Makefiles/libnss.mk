
include Makefiles/base.mk
include Makefiles/libnss-defs.mk

$(libnss_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(libnss_CFLAGS)

build: $(SO_NAME_NSS)

$(SO_NAME_NSS): $(libnss_OBJS)
	@echo Link $(libnss_TARGET)
	$(CC) -o $(SO_NAME_NSS) $(libnss_OBJS) $(libnss_LDFLAGS)

install_libnss:
	if [ -f $(SO_NAME_NSS) ]; then \
	    mkdir -p $(INSTALL_DIR)/lib; \
	    cp $(SO_NAME_NSS) $(INSTALL_DIR)/lib; \
	fi

uninstall_libnss:
	if [ -f $(INSTALL_DIR)/lib/$(SO_NAME_NSS) ]; then \
	    rm -f $(INSTALL_DIR)/lib/$(SO_NAME_NSS); \
	fi

flags:
	@echo Build libnss
	@echo CFLAGS = $(libnss_CFLAGS)
	@echo LDFLAGS = $(libnss_LDFLAGS)
	@echo

