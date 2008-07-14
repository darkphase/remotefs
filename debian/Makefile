
RELEASE_CFLAGS = -s -O2

default: all
all: rfsdeb rfsddeb

rfsdeb:
	@CFLAGS="$(RELEASE_CFLAGS)" $(MAKE) -f Makefiles/Makefile rfs
	@$(MAKE) -f debian/rfs.mk

rfsddeb:
	@CFLAGS="$(RELEASE_CFLAGS)" $(MAKE) -f Makefiles/Makefile rfsd
	@CFLAGS="$(RELEASE_CFLAGS)" $(MAKE) -f Makefiles/Makefile rfspasswd
	@$(MAKE) -f debian/rfsd.mk

clean:
	@rm -fr dpkg
	
mrproper: clean
	@rm -fr *.deb
