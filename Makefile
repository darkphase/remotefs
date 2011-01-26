#############################
# Verbosity
#############################

include build/Makefiles/variable/verbosity.mk

#############################
# OS-dependent options
#############################

default: help

# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname

include build/Makefiles/$(OS)$(ALT).mk
include build/Makefiles/options.mk
include build/Makefiles/shortcuts.mk
include build/Makefiles/packaging/installing.mk

#############################
# Compile flags
#############################

CFLAGS_MAIN_DEBUG    = "$(CFLAGS_DEBUG) -DRFS_DEBUG"
CFLAGS_MAIN_RELEASE  = "$(CFLAGS_RELEASE)"
LDFLAGS_MAIN_DEBUG   = "$(LDFLAGS_DEBUG)"
LDFLAGS_MAIN_RELEASE = "$(LDFLAGS_RELEASE)"

#############################
# Help
#############################

help:
	@more build/Makefiles/help

#############################
# Debug/Release
#############################

debug: dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_DEBUG) LDFLAGS_MAIN=$(LDFLAGS_MAIN_DEBUG) $(MAKE) $(SILENT) -f build/Makefiles/base.mk $(ALL)

all release: $(ALL)

#############################
# General targets
#############################

rfs: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfs
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build

librfs: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk librfs
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build

rfsd: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsd
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build

rfspasswd: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfspasswd
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build

libnss: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk libnss
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build

nssd: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk nssd
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean_build

dummy:
	@$(MAKE) $(SILENT) -f build/Makefiles/version.mk make_version

#######################################
# Redirect for cleaning, depends and version
#######################################
clean:
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk clean

depends:
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk depends

force_version:
	@$(MAKE) $(SILENT) -f build/Makefiles/version.mk force_version

########################################
# Redirects for packaging
########################################

rfsrpm: force_version dummy
	@ARCH=`rpm --eval "%{_arch}"` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsrpm
rfsdrpm: force_version dummy
	@ARCH=`rpm --eval "%{_arch}"` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsdrpm
rfsnssrpm: force_version dummy
	@ARCH=`rpm --eval "%{_arch}"` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsnssrpm
rpms: rfsrpm rfsdrpm rfsnssrpm

rfsdeb: force_version dummy
	@ARCH=`dpkg --print-architecture` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsdeb
rfsddeb: force_version dummy
	@ARCH=`dpkg --print-architecture` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsddeb
rfsnssdeb: force_version dummy
	@ARCH=`dpkg --print-architecture` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsnssdeb
debs: rfsdeb rfsddeb rfsnssdeb

rfsdipk: force_version dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsdipk
	
ipks: rfsdipk

rfsdebuild: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsdebuild
rfsebuild: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsebuild
rfsnssebuild: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk rfsnssebuild
ebuilds: rfsdebuild rfsebuild rfsnssebuild

tbz: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk tbz

########################################
# Redirects for man
########################################

rfs_man: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/man.mk rfs_man
	
rfsd_man: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/man.mk rfsd_man
	
rfsnss_man: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/man.mk rfsd_man
	
man: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/man.mk man

include build/Makefiles/variable/custom.mk
