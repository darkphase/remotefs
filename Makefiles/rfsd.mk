
include Makefiles/base.mk
include Makefiles/rfsd-defs.mk

$(rfsd_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(rfsd_CFLAGS)

build: $(rfsd_OBJS)
	@echo Link $(rfsd_TARGET)
	$(CC) -o $(rfsd_TARGET) $(rfsd_OBJS) $(rfsd_LDFLAGS)

install_rfsd:
	@if [ -f $(rfsd_TARGET) ]; then cp $(rfsd_TARGET) $(TARGET_DIR); fi

flags:
	@echo Build rfsd
	@echo CFLAGS = $(rfsd_CFLAGS)
	@echo LDFLAGS = $(rfsd_LDFLAGS)
	@echo
