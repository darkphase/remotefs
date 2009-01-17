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

librfsd: dummy
	@CFLAGS_MAIN=$(CFLAGS_MAIN_RELEASE) LDFLAGS_MAIN=$(LDFLAGS_MAIN_RELEASE) $(MAKE) -sf Makefiles/base.mk librfsd
	
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

rfs_nss: dummy
	@$(MAKE) -sf nss/Makefile NSSPATH=nss/

dummy:

#######################################
# Rules for cleaning,and dpendencied
#######################################
clean:
	@$(MAKE) -sf Makefiles/base.mk clean

clea-rfs_nss:
	@$(MAKE) -sf nss/Makefile NSSPATH=nss/ clean

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

tbz: dummy
	@$(MAKE) -sf Makefiles/base.mk tbz
	
packages: debs rpms tbz 
	@$(MAKE) -sf Makefiles/base.mk clean_bins

install: install-rfs_nss
	@$(MAKE) -sf Makefiles/base.mk install

install-rfs_nss:
	@$(MAKE) -sf nss/Makefile NSSPATH=nss/ install
