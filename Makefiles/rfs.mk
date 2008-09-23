
include Makefiles/base.mk
include Makefiles/rfs-defs.mk

$(rfs_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(rfs_CFLAGS) $(DRF) $(OPTS)

build: $(rfs_OBJS)
	@echo Link $(rfs_TARGET)
	$(CC) -o $(rfs_TARGET) $(rfs_OBJS) $(rfs_LDFLAGS)
	
