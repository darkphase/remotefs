
include Makefiles/base.mk
include Makefiles/rfspasswd-defs.mk

$(rfspasswd_OBJS):
	$(CC) -c -o $@ $*.c $(rfspasswd_CFLAGS) $(DRF) $(OPTS)

build: $(rfspasswd_OBJS)
	$(CC) -o $(rfspasswd_TARGET) $(rfspasswd_OBJS) $(rfspasswd_LDFLAGS)
