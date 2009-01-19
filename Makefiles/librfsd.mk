
include Makefiles/base.mk
include Makefiles/librfsd-defs.mk

$(librfsd_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(librfsd_CFLAGS)

build: $(SO_NAME)

$(SO_NAME): $(librfsd_OBJS)
	@echo Link $(@)
	$(CC) -o $(SO_NAME) $(librfsd_OBJS) $(librfsd_LDFLAGS)
	$(LN) $(@) $(librfsd_TARGET).$(SO_EXT)

install_librfsd:
	@if [ -f $(SO_NAME) ]; then cp $(SO_NAME) $(TARGET_DIR); fi
	@( cd $(TARGET_DIR); $(LN) -sf $(SO_NAME) $(librfsd_TARGET).$(SO_EXT) )

flags:
	@echo Build librfsd
	@echo CFLAGS = $(librfsd_CFLAGS)
	@echo LDFLAGS = $(librfsd_LDFLAGS)
	@echo
