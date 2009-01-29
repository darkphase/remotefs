
include Makefiles/base.mk
include Makefiles/librfs-defs.mk

$(librfs_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(librfs_CFLAGS)

build: $(SO_NAME)

$(SO_NAME): $(librfs_OBJS)
	@echo Link $(librfs_TARGET)
	$(CC) -o $(SO_NAME) $(librfs_OBJS) $(librfs_LDFLAGS)
	$(LN) $(SO_NAME) $(librfs_TARGET).$(SO_EXT)

install_librfs:
	@if [ -f $(SO_NAME) ]; then cp $(SO_NAME) $(TARGET_DIR); fi
	@( cd  $(TARGET_DIR); $(LN) $(SO_NAME) $(librfs_TARGET).$(SO_EXT) )

flags:
	@echo Build librfs
	@echo CFLAGS = $(librfs_CFLAGS)
	@echo LDFLAGS = $(librfs_LDFLAGS)
	@echo
