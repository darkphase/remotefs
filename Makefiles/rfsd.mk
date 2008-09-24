
include Makefiles/base.mk
include Makefiles/rfsd-defs.mk

$(rfsd_OBJS):
	@echo Compile $@
	@$(CC) -c -o $@ $*.c $(rfsd_CFLAGS) $(DRF) $(OPTS)

build: $(rfsd_OBJS)
	@echo Link $(rfsd_TARGET)
	@$(CC) -o $(rfsd_TARGET) $(rfsd_OBJS) $(rfsd_LDFLAGS)

flags:
	@echo Build rfsd
	@echo CFLAGS = $(rfsd_CFLAGS)
	@echo LDFLAGS = $(rfsd_LDFLAGS)
	@echo
