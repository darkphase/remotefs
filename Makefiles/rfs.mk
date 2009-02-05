
include Makefiles/base.mk
include Makefiles/rfs-defs.mk

$(rfs_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(rfs_CFLAGS)

build: $(rfs_OBJS)
	@echo Link $(rfs_TARGET)
	$(CC) -o $(rfs_TARGET) $(rfs_OBJS) $(rfs_LDFLAGS)

install_rfs:
	if [ -f $(rfs_TARGET) ]; then \
	    mkdir -p $(INSTALL_DIR)/bin; \
	    cp $(rfs_TARGET) $(INSTALL_DIR)/bin; \
	fi
	
uninstall_rfs:
	if [ -f $(INSTALL_DIR)/bin/$(rfs_TARGET) ]; then \
	    rm -f $(INSTALL_DIR)/bin/$(rfs_TARGET); \
	fi

flags:
	@echo Build rfs
	@echo CFLAGS = $(rfs_CFLAGS)
	@echo LDFLAGS = $(rfs_LDFLAGS)
	@echo
