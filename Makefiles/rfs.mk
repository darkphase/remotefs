
include Makefiles/base.mk
include Makefiles/rfs-defs.mk

$(rfs_OBJS):
	$(CC) -c -o $@ $*.c $(rfs_CFLAGS) $(DRF) $(OPTS)

build: $(rfs_OBJS)
	$(CC) -o $(rfs_TARGET) $(rfs_OBJS) $(rfs_LDFLAGS)
	