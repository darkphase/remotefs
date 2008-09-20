##############################
# Get platform dependent flags
##############################
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname
include Makefiles/$(OS)$(ALT).mk

#############################
# build options
#############################
include Makefiles/options.mk

#############################
# Version for packages
#############################
include Makefiles/version.mk

#############################
# The begin of the world
#############################

#############################
# compile rules for objects
#############################

#######################################
# Rules for compiling programs
#######################################

rfs: dummy
	@echo
	@$(MAKE) -f Makefiles/rfs.mk build
	@echo

rfsd: dummy
	@echo
	@$(MAKE) -f Makefiles/rfsd.mk build
	@echo

rfspasswd: dummy
	@echo
	@$(MAKE) -f Makefiles/rfspasswd.mk build
	@echo

runtests: 
	@$(MAKE) -sC tests run
	@$(MAKE) -sC tests clean


#############################
# remove all temporaries objects
#############################

clean:
	if ls src/*.o >/dev/null 2>&1; then $(RM) -f src/*.o; fi
	if ls *.deb >/dev/null 2>&1; then $(RM) -f *.deb; fi
	if [ -f rfs ]; then $(RM) -f rfs; fi
	if [ -f rfsd ]; then $(RM) -f rfsd; fi
	if [ -f rfspasswd ]; then $(RM) -f rfspasswd; fi

#############################
# Rebuild dependency file
#############################
depends:
	grep 'include *".*"' src/*.c | sed -e 's/\.c/.o/' -e 's/#include *"\(.*\.[ch]\)"/src\/\1/' > Makefiles/depends.mk
	ls src/*.c | sed -e 's/\([^\.]*\)/\1.o:\1/' >> Makefiles/depends.mk

#######################################
# Rules for packaging, ...
#######################################

#############################
# Build deb packages
#############################

rpm: rpm-rfsd rpm-rfs


debrelease: rfsdeb rfsddeb

rfsdeb: rfs
	@CONTROL_TEMPLATE="debian/control.rfs" TARGET="$?" \
	NAME=$< \
	make -f Makefiles/base.mk builddeb

rfsddeb: rfsd rfspasswd
	@CONTROL_TEMPLATE="debian/control.rfsd" TARGET="$?" \
	NAME=$< \
	$(MAKE) -sf Makefiles/base.mk builddeb

builddeb:
	@rm -fr "$(NAME)-$(VERSION)-$(RELEASE)";\
	mkdir "$(NAME)-$(VERSION)-$(RELEASE)";\
	rm -fr dpkg;\
	mkdir -p "dpkg$(INSTALL_DIR)";\
	mkdir -p "dpkg/DEBIAN";\
	sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" $(CONTROL_TEMPLATE) >dpkg/DEBIAN/control.1;\
	SIZE=`cat $(TARGET)|wc -c`; sed -e "s/AND SIZE HERE/$$SIZE/" dpkg/DEBIAN/control.1 >dpkg/DEBIAN/control;\
	rm -f dpkg/DEBIAN/control.1;\
	mv $(TARGET) "dpkg$(INSTALL_DIR)";\
	echo "Building package $(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb";\
	dpkg -b dpkg "$(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb" >/dev/null;\
	rm -fr dpkg;\
	rm -fr "$(NAME)-$(VERSION)-$(RELEASE)"



#############################
# Build RPM
#############################

rpm: rpm-rfsd rpm-rfs

rpm-rfsd: clean
	@RPMNAME=rfsd $(MAKE) -sf Makefiles/base.mk bldrpm
	
rpm-rfs:  clean
	@RPMNAME=rfs $(MAKE) -sf Makefiles/base.mk bldrpm

rpms/%.spec: Makefiles/version.mk
	@echo rebuild spec file
	@sed -e "s/Version:.*/Version:$(VERSION)/"  \
	    -e "s/Release:.*/Release:$(RELEASE)/" $@ \
	    > rpms/tmp; mv rpms/tmp $@


bldrpm: rpms/$(RPMNAME).spec
	@if [ "$(MACHINE)" = "" ] ; then MAC=$(ARCH); else MAC=$(MACHINE); fi ; \
	RPMD=`rpm -q --eval=%{_topdir} --specfile rpms/$(RPMNAME).spec | grep -v $(RPMNAME)`; \
	if [ -d "$$RPMD" -a -w "$$RPMD" ];\
	then \
	   VER=`sed -n 's/Version:\(.*\)/\1/p' rpms/$(RPMNAME).spec|tr -d ' '`; \
	   SDIR=`pwd`; \
	   NM=$(RPMNAME)-$$VER; \
	   mkdir /tmp/$$NM ;\
	   cp -r * /tmp/$$NM; \
	   find /tmp/$$NM -name .svn -exec rm -fr {} \; 2>/dev/null; \
	   cd /tmp ; \
	   tar czf $$RPMD/SOURCES/$$NM.tar.gz $$NM ; \
	   cd $$SDIR ; \
	   if [ -x /usr/bin/rpmbuild ]; \
	   then \
	      rpmbuild -ba --target $$MAC /tmp/$$NM/rpms/$(RPMNAME).spec; \
	   else \
	      rpm -ba $$NM/rpms/$(RPMNAME).spec; \
	   fi; \
	   rm -fr /tmp/$$NM 2>/dev/tty 1>/dev/tty; \
	else \
	   echo You must be root for this.; \
	   exit 2; \
	fi;\
	
dummy:

#############################
# Dependencies for all proj.
#############################

include Makefiles/depends.mk
