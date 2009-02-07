##############################
# Get degug and optim. flags
##############################
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname
include Makefiles/$(OS)$(ALT).mk
include Makefiles/options.mk

#############################
# The begin of the world
#############################

CFLAGS_MAIN_DEBUG    = "$(CFLAGS_DEBUG) -DRFS_DEBUG"
CFLAGS_MAIN_RELEASE  = "$(CFLAGS_RELEASE)"
LDFLAGS_MAIN_DEBUG   = "$(LDFLAGS_DEBUG)"
LDFLAGS_MAIN_RELEASE = "$(LDFLAGS_RELEASE)"

help:
	@more Makefiles/help

debug:
	@CFLAGS_MAIN=$(CFLAGS_MAIN_DEBUG) LDFLAGS_MAIN=$(LDFLAGS_MAIN_DEBUG) $(MAKE) -sf Makefiles/base.mk rfsd rfspasswd rfs

all release: server client

server: rfsd rfspasswd

client: rfs

rfs: dummy
	@$(MAKE) -sf Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk rfs
	@$(MAKE) -sf Makefiles/base.mk clean_build

librfs: dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk librfs

rfsd: dummy
	@$(MAKE) -sf Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk rfsd
	@$(MAKE) -sf Makefiles/base.mk clean_build

rfspasswd: dummy
	@$(MAKE) -sf Makefiles/base.mk clean_build
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk rfspasswd
	@$(MAKE) -sf Makefiles/base.mk clean_build

dummy:

#######################################
# Rules for cleaning,and dpendencied
#######################################
clean:
	@$(MAKE) -sf Makefiles/base.mk clean

depends:
	@$(MAKE) -f Makefiles/base.mk depends

########################################
# Rules for packaging, ...
########################################

rfsrpm: dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk rfsrpm
rfsdrpm: dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk rfsdrpm
rpms: rfsrpm rfsdrpm

rfsdeb: dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk rfsdeb $(TGTARCH)
rfsddeb: dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk rfsddeb $(TGTARCH)
debs: rfsdeb rfsddeb

rfsdipk: dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk rfsdipk $(TGTARCH)
	
ipks: rfsdipk

tbz: dummy
	@$(MAKE) -sf Makefiles/base.mk tbz
	
packages: tbz debs rpms
	@ALT=MIPS   $(MAKE) -s ipks
	@ALT=MIPSEL $(MAKE) -s ipks
	@ALT=PPC    $(MAKE) -s ipks
	@ALT=ARM    $(MAKE) -s ipks
	@ALT=ARMEB  $(MAKE) -s ipks

install_client:
	@$(MAKE) -sf Makefiles/librfs.mk install_librfs
	@$(MAKE) -sf Makefiles/rfs.mk install_rfs
install_server:
	@$(MAKE) -sf Makefiles/rfsd.mk install_rfsd
	@$(MAKE) -sf Makefiles/rfspasswd.mk install_rfspasswd
install: dummy install_client install_server
	@$(MAKE) -sf Makefiles/base.mk install_man

uninstall_client:
	@$(MAKE) -sf Makefiles/librfs.mk uninstall_librfs
	@$(MAKE) -sf Makefiles/rfs.mk uninstall_rfs
uninstall_server:
	@$(MAKE) -sf Makefiles/rfsd.mk uninstall_rfsd
	@$(MAKE) -sf Makefiles/rfspasswd.mk uninstall_rfspasswd
uninstall: dummy uninstall_client uninstall_server
	@$(MAKE) -sf Makefiles/base.mk uninstall_man

rfs_man:
	@$(MAKE) -sf Makefiles/base.mk rfs_man
rfsd_man:
	@$(MAKE) -sf Makefiles/base.mk rfsd_man
man: dummy rfs_man rfsd_man
