
DEBUG_CFLAGS=-DRFS_DEBUG -g

all: rfs rfsd rfspasswd
default: debug

debug:
	@CFLAGS="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/Makefile
	
release: rfsdeb rfsddeb

rfs:
	@CFLAGS="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/Makefile rfs
	
rfsd:
	@CFLAGS="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/Makefile rfsd
	
rfspasswd:
	@CFLAGS="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/Makefile rfspasswd
	
rfsdeb:	cleanup
	@$(MAKE) -sf debian/Makefile rfsdeb
	
rfsddeb: cleanup
	@$(MAKE) -sf debian/Makefile rfsddeb
	
runtests: 
	@$(MAKE) -sC tests run
	@$(MAKE) -sC tests clean
	
cleanup:
	@$(MAKE) -sf Makefiles/Makefile clean
	@$(MAKE) -sf debian/Makefile clean
	
clean:
	@$(MAKE) -sf Makefiles/Makefile clean
	@$(MAKE) -sf debian/Makefile mrproper
