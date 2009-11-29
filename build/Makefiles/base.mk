
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname

include build/Makefiles/$(OS)$(ALT).mk
include build/Makefiles/options.mk
include build/Makefiles/version.mk
include build/Makefiles/shortcuts.mk
include build/Makefiles/variable/verbosity.mk
include build/Makefiles/variable/custom.mk

libnss: dummy
	@echo
	@$(MAKE) -f build/Makefiles/libnss.mk flags build
	@echo

nssd: dummy
	@echo
	@$(MAKE) -f build/Makefiles/nssd.mk flags build
	@echo

librfs: dummy
	@echo
	@$(MAKE) -f build/Makefiles/librfs.mk flags build
	@echo

rfs: dummy librfs
	@echo
	@$(MAKE) -f build/Makefiles/rfs.mk flags build
	@echo

rfsd: dummy
	@echo
	@$(MAKE) -f build/Makefiles/rfsd.mk flags build
	@echo

rfspasswd: dummy
	@echo
	@$(MAKE) -f build/Makefiles/rfspasswd.mk flags build
	@echo

#############################
# remove all temporary objects
#############################

clean_build: dummy
	$(RM) -f src/*.o
	$(RM) -f src/acl/*.o
	$(RM) -f src/md5crypt/*.o
	$(RM) -f src/nss/*.o
	$(RM) -f src/resume/*.o
	$(RM) -f src/ssl/*.o
	$(RM) -f rfs_nss/src/*.o

clean_bins: dummy
	$(RM) -f rfs
	$(RM) -f rfsd
	$(RM) -f rfspasswd
	$(RM) -f rfs_nssd
	$(RM) -f *.so*

clean_version: 
	$(RM) -f src/version.h

clean_tmp: dummy clean_packages_tmp clean_build
clean: clean_build clean_bins clean_packages clean_version

#############################
# Rebuild dependency file
#############################
builddep: dummy
	grep -E '#\s*include[^"]+"[^"]+"' $(SCANDIR)*.c | sed -r -e 's/\.c/.o/' -e 's/#\s*include[^"]+"([^"]+)".*/$(SCANDIR)\1/' >> build/Makefiles/depends.mk
	ls $(SCANDIR)*.c | sed -e 's/\([^\.]*\)/\1.o:\1/' >> build/Makefiles/depends.mk

depends:
	echo -n "" > build/Makefiles/depends.mk
	
	SCANDIR="src\/"            $(MAKE) -f build/Makefiles/base.mk builddep
	SCANDIR="src\/acl\/"       $(MAKE) -f build/Makefiles/base.mk builddep
	SCANDIR="src\/md5crypt\/"  $(MAKE) -f build/Makefiles/base.mk builddep
	SCANDIR="src\/nss\/"       $(MAKE) -f build/Makefiles/base.mk builddep
	SCANDIR="src\/resume\/"    $(MAKE) -f build/Makefiles/base.mk builddep
	SCANDIR="src\/ssl\/"       $(MAKE) -f build/Makefiles/base.mk builddep
	SCANDIR="rfs_nss\/src\/"   $(MAKE) -f build/Makefiles/base.mk builddep

########################################
# Packaging
########################################

include build/Makefiles/packages.mk

########################################
# man
########################################

include build/Makefiles/man.mk

#############################
# Dummy target
#############################

dummy:

#############################
# Dependencies for all proj.
#############################

include build/Makefiles/depends.mk

