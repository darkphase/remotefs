
include Makefiles/base.mk
include Makefiles/librfsd-defs.mk

$(librfsd_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(librfsd_CFLAGS) $(OPTS)

build: $(SO_NAME)

$(SO_NAME): $(librfsd_OBJS)
	@echo Link $(@)
	$(CC) -o $(SO_NAME) $(librfsd_OBJS) $(librfsd_LDFLAGS)
	$(LN) $(@) $(librfsd_TARGET).$(SO_EXT)

install_librfsd:
	@if [ -f $(librfsd_TARGET) ]; then cp $(librfsd_TARGET) $(TARGET_DIR); fi
	$(LN) $(SO_NAME) $(librfsd_TARGET).so

flags:
	@echo Build librfsd
	@echo CFLAGS = $(librfsd_CFLAGS) $(OPTS)
	@echo LDFLAGS = $(librfsd_LDFLAGS)
	@echo
