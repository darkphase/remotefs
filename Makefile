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

DEBFLAG = $(CFLAGS_DBG) -DRFS_DEBUG
RELFLAG = $(CFLAGS_OPT)

help:
	@cat Makefiles/help

debug:
	echo $(DEBFLAG)
	@DRF="$(DEBFLAG)" make -sf Makefiles/base.mk rfs rfsd rfspasswd

release:
	echo $(DEBFLAG)
	@DRF="$(RELFLAG)" make -sf Makefiles/base.mk rfs rfsd rfspasswd

all:
	@if [ "$DRF" = "" -a "$$DRF" = "" ] ; then \
		DRF="$(DEBFLAG)" make -ss Makefiles/base.mk rfs rfsd rfspasswd; \
	else \
		DRF="$(RELFLAG)" make -sf Makefiles/base.mk rfs rfsd rfspasswd; \
	fi

#######################################
# Rules for compiling, ...
#######################################

rfs: dummy
	@if [ "$DRF" = "" -a "$$DRF" = "" ] ; then \
		DRF="$(DEBFLAG)" make -sf Makefiles/base.mk rfs; \
	else \
		DRF="$(RELFLAG)" make -sf Makefiles/base.mk rfs; \
	fi

rfsd: dummy
	@if [ "$DRF" = "" -a "$$DRF" = "" ] ; then \
		DRF="$(DEBFLAG)" make -sf Makefiles/base.mk rfsd; \
	else \
		DRF="$(RELFLAG)" make -sf Makefiles/base.mk rfsd; \
	fi

rfspasswd: dummy
	@if [ "$DRF" = "" -a "$$DRF" = "" ] ; then \
		DRF="$(DEBFLAG)" make -sf Makefiles/base.mk rfspasswd; \
	else \
		DRF="$(RELFLAG)" make -sf Makefiles/base.mk rfspasswd; \
	fi

dummy:

#######################################
# Rules for cleaning,and dpendencied
#######################################
clean:
	@DRF="$(DEBFLAG)" make -sf Makefiles/base.mk clean

depends:
	@make -sf Makefiles/base.mk depends

########################################
# Rules for packaging, ...
########################################

rpm: clean
	@DRF="$(RELFLAG)" make -sf Makefiles/base.mk rpm
rpm-rfs: clean
	@DRF="$(RELFLAG)" make -sf Makefiles/base.mk rpm-rfs
rpm-rfsd: clean
	@DRF="$(RELFLAG)" make -sf Makefiles/base.mk rpm-rfsd

rfsdeb: clean
	@DRF="$(RELFLAG)" $(MAKE) -sf Makefiles/base.mk rfsdeb $(TGTARCH)
rfsddeb: clean
	@DRF="$(RELFLAG)" $(MAKE) -sf Makefiles/base.mk rfsddeb $(TGTARCH)

runtests: 
	@$(MAKE) -sC tests run
	@$(MAKE) -sC tests clean

#

