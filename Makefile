
#############################
# OS-dependent options
#############################

# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname

include build/Makefiles/$(OS)$(ALT).mk
include build/Makefiles/options.mk
include custom.mk

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
# General targets
#############################

ALL = server client libnss nss

debug: dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_DEBUG) LDFLAGS_MAIN=$(LDFLAGS_MAIN_DEBUG) $(MAKE) -$(V)f build/Makefiles/base.mk rfsd rfspasswd rfs libnss nss

all release: $(ALL) #server client libnss nss

server: rfsd rfspasswd

client: rfs

rfs: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfs
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build

librfs: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk librfs
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build

rfsd: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfsd
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build

rfspasswd: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfspasswd
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build

libnss: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk libnss
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build

nss: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk nss
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean_build

dummy:
	@$(MAKE) -$(V)f build/Makefiles/version.mk make_version

#######################################
# Redirect for cleaning, depends and version
#######################################
clean:
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean

depends:
	@$(MAKE) -$(V)f build/Makefiles/base.mk depends

force_version:
	@$(MAKE) -$(V)f build/Makefiles/version.mk force_version

########################################
# Redirects for packaging
########################################

rfsrpm: force_version dummy
	@ARCH=`rpm --eval "%{_arch}"` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfsrpm
rfsdrpm: force_version dummy
	@ARCH=`rpm --eval "%{_arch}"` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfsdrpm
rfsnssrpm: force_version dummy
	@ARCH=`rpm --eval "%{_arch}"` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfsnssrpm
rpms: rfsrpm rfsdrpm rfsnssrpm

rfsdeb: force_version dummy
	@ARCH=`dpkg --print-architecture` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfsdeb
rfsddeb: force_version dummy
	@ARCH=`dpkg --print-architecture` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfsddeb
rfsnssdeb: force_version dummy
	@ARCH=`dpkg --print-architecture` \
	CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfsnssdeb
debs: rfsdeb rfsddeb rfsnssdeb

rfsdipk: force_version dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -$(V)f build/Makefiles/base.mk rfsdipk
	
ipks: rfsdipk

rfsdebuild: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk rfsdebuild
rfsebuild: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk rfsebuild
rfssslebuild: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk rfssslebuild
rfsnssebuild: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk rfsnssebuild
ebuilds: rfsdebuild rfsebuild rfssslebuild rfsnssebuild

tbz: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk tbz

install: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk install
install_nss: dummy 
	@$(MAKE) -$(V)f build/Makefiles/base.mk install_nss

uninstall: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk uninstall
uninstall_nss: dummy
	@$(MAKE) -$(V)f build/Makefiles/base.mk uninstall_nss

########################################
# Redirects for man
########################################

rfs_man: dummy
	@$(MAKE) -$(V)f build/Makefiles/man.mk rfs_man
	
rfsd_man: dummy
	@$(MAKE) -$(V)f build/Makefiles/man.mk rfsd_man
	
rfsnss_man: dummy
	@$(MAKE) -$(V)f build/Makefiles/man.mk rfsd_man
	
man: dummy
	@$(MAKE) -$(V)f build/Makefiles/man.mk man
