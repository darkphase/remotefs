
include Makefiles/base.mk
include Makefiles/librfs-defs.mk

$(librfs_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(librfs_CFLAGS) $(OPTS)

build: $(SO_NAME)

$(SO_NAME): $(librfs_OBJS)
	@echo Link $(librfs_TARGET)
	$(CC) -o $(SO_NAME) $(librfs_OBJS) $(librfs_LDFLAGS)
	$(LN) $(SO_NAME) $(librfs_TARGET).$(SO_EXT)

install_rfs:
	@if [ -f $(librfs_TARGET) ]; then cp $(librfs_TARGET) $(TARGET_DIR); fi
	$(LN) $(SO_NAME) $(librfs_TARGET).$(SO_EXT)

flags:
	@echo Build librfs
	@echo CFLAGS = $(librfs_CFLAGS) $(OPTS)
	@echo LDFLAGS = $(librfs_LDFLAGS)
	@echo
