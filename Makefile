##############################
# Get degug and optim. flags
##############################
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname

# user-specific make targets
CUSTOM_MK=custom.mk
ifeq ($(V), 99)
export V=
export OUTPUT=&1
else
export V=s
export OUTPUT="/dev/null"
endif

# reset component to build

ALL = server client libnss nss
include build/Makefiles/$(OS)$(ALT).mk
include build/Makefiles/options.mk

#############################
# The begin of the world
#############################

CFLAGS_MAIN_DEBUG    = "$(CFLAGS_DEBUG) -DRFS_DEBUG"
CFLAGS_MAIN_RELEASE  = "$(CFLAGS_RELEASE)"
LDFLAGS_MAIN_DEBUG   = "$(LDFLAGS_DEBUG)"
LDFLAGS_MAIN_RELEASE = "$(LDFLAGS_RELEASE)"

help:
	@more build/Makefiles/help

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
# Rules for cleaning,and dpendencied
#######################################
clean:
	@$(MAKE) -$(V)f build/Makefiles/base.mk clean

depends:
	@$(MAKE) -f build/Makefiles/base.mk depends

force_version:
	@$(MAKE) -$(V)f build/Makefiles/version.mk force_version

########################################
# Rules for packaging, ...
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
	
install_client:
	@$(MAKE) -$(V)f build/Makefiles/librfs.mk install_librfs
	@$(MAKE) -$(V)f build/Makefiles/rfs.mk install_rfs
install_server:
	@$(MAKE) -$(V)f build/Makefiles/rfsd.mk install_rfsd
	@$(MAKE) -$(V)f build/Makefiles/rfspasswd.mk install_rfspasswd
install: dummy install_client install_server
	@$(MAKE) -$(V)f build/Makefiles/base.mk install_man

install_nss: dummy 
	@$(MAKE) -$(V)f build/Makefiles/libnss.mk install_libnss
	@$(MAKE) -$(V)f build/Makefiles/nssd.mk install_nssd
	@$(MAKE) -$(V)f build/Makefiles/base.mk install_man

uninstall_client:
	@$(MAKE) -$(V)f build/Makefiles/librfs.mk uninstall_librfs
	@$(MAKE) -$(V)f build/Makefiles/rfs.mk uninstall_rfs
uninstall_server:
	@$(MAKE) -$(V)f build/Makefiles/rfsd.mk uninstall_rfsd
	@$(MAKE) -$(V)f build/Makefiles/rfspasswd.mk uninstall_rfspasswd
uninstall: dummy uninstall_client uninstall_server
	@$(MAKE) -$(V)f build/Makefiles/base.mk uninstall_man

uninstall_nss: dummy
	@$(MAKE) -$(V)f build/Makefiles/nssd.mk uninstall_nssd
	@$(MAKE) -$(V)f build/Makefiles/libnss.mk uninstall_libnss
	@$(MAKE) -$(V)f build/Makefiles/base.mk uninstall_man

rfs_man:
	@$(MAKE) -$(V)f build/Makefiles/base.mk rfs_man
rfsd_man:
	@$(MAKE) -$(V)f build/Makefiles/base.mk rfsd_man
rfsnss_man:
	@$(MAKE) -$(V)f build/Makefiles/base.mk rfsnss_man
man: dummy rfs_man rfsd_man rfsnss_man

# This don't work with FreeBSD and Solaris
#-include $(CUSTOM_MK)

