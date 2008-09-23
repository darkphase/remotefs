
include Makefiles/base.mk
include Makefiles/rfspasswd-defs.mk

$(rfspasswd_OBJS):
	@echo Compile $@
	@$(CC) -c -o $@ $*.c $(rfspasswd_CFLAGS) $(DRF) $(OPTS)

build: $(rfspasswd_OBJS)
	@echo Link $(rfspasswd_TARGET)
	@$(CC) -o $(rfspasswd_TARGET) $(rfspasswd_OBJS) $(rfspasswd_LDFLAGS)
