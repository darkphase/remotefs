
DEBUG_CFLAGS = -DRFS_DEBUG -g -DFUSE_USE_VERSION=25 `pkg-config --cflags fuse`

all: default
default: debug

debug:
	@CFLAGS="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/Makefile

rfs:
	@CFLAGS="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/Makefile rfs
	
rfsd:
	@CFLAGS="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/Makefile rfsd
	
rfspasswd:
	@CFLAGS="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/Makefile rfspasswd
	
release: rfsdeb rfsddeb
	
rfsdeb:	cleanup
	@$(MAKE) -sf debian/Makefile rfsdeb
	
rfsddeb: cleanup
	@$(MAKE) -sf debian/Makefile rfsddeb
	
cleanup:
	@$(MAKE) -sf Makefiles/Makefile clean
	@$(MAKE) -sf debian/Makefile clean
	
clean:
	@$(MAKE) -sf Makefiles/Makefile clean
	@$(MAKE) -sf debian/Makefile mrproper
