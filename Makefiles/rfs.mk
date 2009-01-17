
include Makefiles/base.mk
include Makefiles/rfs-defs.mk

$(rfs_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(rfs_CFLAGS)

build: $(rfs_OBJS)
	@echo Link $(rfs_TARGET)
	$(CC) -o $(rfs_TARGET) $(rfs_OBJS) $(rfs_LDFLAGS)

install_rfs:
	@if [ -f $(rfs_TARGET) ]; then cp $(rfs_TARGET) $(TARGET_DIR); fi

flags:
	@echo Build rfs
	@echo CFLAGS = $(rfs_CFLAGS)
	@echo LDFLAGS = $(rfs_LDFLAGS)
	@echo
