
include Makefiles/base.mk
include Makefiles/rfsd-defs.mk

$(rfsd_OBJS):
	$(CC) -c -o $@ $*.c $(rfsd_CFLAGS) $(DRF) $(OPTS)

build: $(rfsd_OBJS)
	$(CC) -o $(rfsd_TARGET) $(rfsd_OBJS) $(rfsd_LDFLAGS)
