
include build/Makefiles/base.mk
include build/Makefiles/rfspasswd-defs.mk

$(rfspasswd_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(rfspasswd_CFLAGS)

build: $(rfspasswd_OBJS)
	@echo Link $(rfspasswd_TARGET)
	$(CC) -o $(rfspasswd_TARGET) $(rfspasswd_OBJS) $(rfspasswd_LDFLAGS)

install_rfspasswd:
	if [ -f $(rfspasswd_TARGET) ]; then \
	    mkdir -p $(INSTALL_DIR)/bin; \
	    cp $(rfspasswd_TARGET) $(INSTALL_DIR)/bin; \
	fi

uninstall_rfspasswd:
	if [ -f $(INSTALL_DIR)/bin/$(rfspasswd_TARGET) ]; then \
	    rm -f $(INSTALL_DIR)/bin/$(rfspasswd_TARGET); \
	fi

flags:
	@echo Build rfspasswd
	@echo CFLAGS = $(rfspasswd_CFLAGS)
	@echo LDFLAGS = $(rfspasswd_LDFLAGS)
	@echo
	
