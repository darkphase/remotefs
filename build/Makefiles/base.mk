
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname

include build/Makefiles/$(OS)$(ALT).mk
include build/Makefiles/options.mk
include build/Makefiles/version.mk

INC_CUSTOM=$(shell build/Makefiles/check_custom.sh)
INC_CUSTOM:sh=build/Makefiles/check_custom.sh
include $(INC_CUSTOM)

libnss: dummy
	@echo ZZZ $(INC_CUSTOM)
	@$(MAKE) -f build/Makefiles/libnss.mk flags build
	@echo

nss: dummy
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
	$(RM) -f src/md5crypt/*.o
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
depends:
	@touch build/Makefiles/depends.mk
	@grep -E '#\s*include[^"]+"[^"]+"' src/*.c | sed -r -e 's/\.c/.o/' -e 's/#\s*include[^"]+"([^"]+)".*/src\/\1/' > build/Makefiles/depends.mk
	@grep -E '#\s*include[^"]+"[^"]+"' rfs_nss/src/*.c | sed -r -e 's/\.c/.o/' -e 's/#\s*include[^"]+"([^"]+)".*/rfs_nss\/src\/\1/' >> build/Makefiles/depends.mk
	@ls src/*.c | sed -e 's/\([^\.]*\)/\1.o:\1/' >> build/Makefiles/depends.mk
	@ls rfs_nss/src/*.c | sed -e 's/\([^\.]*\)/\1.o:\1/' >> build/Makefiles/depends.mk

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

