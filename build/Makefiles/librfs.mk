
include build/Makefiles/base.mk
include build/Makefiles/librfs-defs.mk

$(librfs_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(librfs_CFLAGS)

build: $(SO_NAME)

$(SO_NAME): $(librfs_OBJS)
	@echo Link $(librfs_TARGET)
	$(CC) -o $(SO_NAME) $(librfs_OBJS) $(librfs_LDFLAGS)
	$(LN) $(SO_NAME) $(librfs_TARGET).$(SO_EXT)

install_librfs:
	if [ -f $(SO_NAME) ]; then \
	    mkdir -p $(INSTALL_DIR)/lib; \
	    cp $(SO_NAME) $(INSTALL_DIR)/lib; \
	fi

uninstall_librfs:
	if [ -f $(INSTALL_DIR)/lib/$(SO_NAME) ]; then \
	    rm -f $(INSTALL_DIR)/lib/$(SO_NAME); \
	fi

flags:
	@echo Build librfs
	@echo CFLAGS = $(librfs_CFLAGS)
	@echo LDFLAGS = $(librfs_LDFLAGS)
	@echo
