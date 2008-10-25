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
	@$(MAKE) -f Makefiles/rfs.mk flags build
	@echo

rfsd: dummy
	@echo
	@$(MAKE) -sf Makefiles/rfsd.mk flags build
	@echo

rfspasswd: dummy
	@echo
	@$(MAKE) -sf Makefiles/rfspasswd.mk flags build
	@echo

runtests: 
	@$(MAKE) -sC tests run
	@$(MAKE) -sC tests clean


#############################
# remove all temporaries objects
#############################

clean:
	@if ls src/*.o >/dev/null 2>&1; then $(RM) -f src/*.o; fi
	@if ls *.deb >/dev/null 2>&1; then $(RM) -f *.deb; fi
	@if [ -f rfs ]; then $(RM) -f rfs; fi
	@if [ -f rfsd ]; then $(RM) -f rfsd; fi
	@if [ -f rfspasswd ]; then $(RM) -f rfspasswd; fi
	@if [ -f remotefs-${VERSION}_${RELEASE}.tar.bz2 ]; then $(RM) -f remotefs-${VERSION}_${RELEASE}.tar.bz2; fi
	@if [ -d man/fz ]; then $(RM) -fr man/gz; fi
	@$(RM) -fr man/gz
	@$(RM) -fr dpkg/ dpkg_man/ dpkg_etc/

#############################
# Rebuild dependency file
#############################
depends:
	@grep 'include *".*"' src/*.c | sed -e 's/\.c/.o/' -e 's/#include *"\(.*\.[ch]\)"/src\/\1/' > Makefiles/depends.mk
	@ls src/*.c | sed -e 's/\([^\.]*\)/\1.o:\1/' >> Makefiles/depends.mk

#######################################
# Rules for packaging, ...
#######################################

#############################
# Build man pages
#############################

man: dummy
	@mkdir -p man/gz/man1
	@mkdir -p man/gz/man8
	@gzip -c < man/rfs.1 > man/gz/man1/rfs.1.gz
	@gzip -c < man/rfsd.8 > man/gz/man8/rfsd.8.gz
	@gzip -c < man/rfspasswd.8 > man/gz/man8/rfspasswd.8.gz

#############################
# Build tarball
#############################

tbz: clean
	@echo "Building remotefs-${VERSION}_${RELEASE}.tar.bz2"
	@mkdir -p "remotefs-$(VERSION)_$(RELEASE)/src/"
	@mkdir -p "remotefs-$(VERSION)_$(RELEASE)/man/"
	@mkdir -p "remotefs-$(VERSION)_$(RELEASE)/Makefiles/"
	@cp LICENSE "remotefs-$(VERSION)_$(RELEASE)/" 2> /dev/null
	@cp AUTHORS "remotefs-$(VERSION)_$(RELEASE)/" 2> /dev/null
	@cp CHANGELOG "remotefs-$(VERSION)_$(RELEASE)/" 2> /dev/null
	@cp src/*.c src/*.h "remotefs-$(VERSION)_$(RELEASE)/src" 2> /dev/null
	@cp man/*.1 man/*.8 "remotefs-$(VERSION)_$(RELEASE)/man/" 2> /dev/null
	@cp Makefiles/* "remotefs-$(VERSION)_$(RELEASE)/Makefiles/" 2> /dev/null
	@mkdir -p "remotefs-$(VERSION)_$(RELEASE)/debian"
	@mkdir -p "remotefs-$(VERSION)_$(RELEASE)/rpms"
	@mkdir -p "remotefs-$(VERSION)_$(RELEASE)/etc"
	@mkdir -p "remotefs-$(VERSION)_$(RELEASE)/init.d"
	@cp debian/* "remotefs-$(VERSION)_$(RELEASE)/debian/" 2> /dev/null
	@cp rpms/* "remotefs-$(VERSION)_$(RELEASE)/rpms/" 2> /dev/null
	@cp etc/* "remotefs-$(VERSION)_$(RELEASE)/etc/" 2> /dev/null
	@cp init.d/* "remotefs-$(VERSION)_$(RELEASE)/init.d/" 2> /dev/null
	@find remotefs-${VERSION}_${RELEASE} -name .svn -exec rm -fr {} \; 2>/dev/null;
	@cp Makefile "remotefs-$(VERSION)_$(RELEASE)/"
	tar cf - remotefs-${VERSION}_${RELEASE} | bzip2 -c > remotefs-${VERSION}_${RELEASE}.tar.bz2
	rm -fr remotefs-${VERSION}_${RELEASE}

install_man:
	@INSTALL_DIR=$(INSTALL_DIR) FILES="man/gz/*"; \
	for GZ_FILE in "$$FILES"; \
	do \
		cp -r $$GZ_FILE $$INSTALL_DIR/share/man; \
	done

install: install_man
	@TARGET_DIR=$(INSTALL_DIR)/bin $(MAKE) -sf Makefiles/rfs.mk install_rfs
	@TARGET_DIR=$(INSTALL_DIR)/bin $(MAKE) -sf Makefiles/rfsd.mk install_rfsd

#############################
# Build deb packages
#############################

debrelease: rfsdeb rfsddeb

rfsmanpages: man
	mkdir -p dpkg_man/man1/
	cp man/gz/man1/rfs.1.gz dpkg_man/man1/

rfsdeb: rfs rfsmanpages
	CONTROL_TEMPLATE="debian/control.rfs" TARGET="rfs" \
	NAME=$< \
	$(MAKE) -sf Makefiles/base.mk builddeb

rfsdmanpages: man
	mkdir -p dpkg_man/man8/
	cp man/gz/man8/rfsd.8.gz dpkg_man/man8/
	cp man/gz/man8/rfspasswd.8.gz dpkg_man/man8/

rfsdetc:
	mkdir -p "dpkg_etc/init.d/";\
	cp etc/rfs-exports "dpkg_etc/";\
	cp init.d/rfsd.debian "dpkg_etc/init.d/rfsd";\
	chmod +x "dpkg_etc/init.d/rfsd"

rfsddeb: rfsd rfspasswd rfsdmanpages rfsdetc
	CONTROL_TEMPLATE="debian/control.rfsd" TARGET="rfsd rfspasswd" \
	NAME=$< \
	$(MAKE) -sf Makefiles/base.mk builddeb

builddeb:
	rm -fr dpkg;\
	mkdir -p "dpkg$(INSTALL_DIR)";\
	mkdir -p "dpkg$(INSTALL_DIR)/bin";\
	mkdir -p "dpkg/DEBIAN";\
	mv $(TARGET) "dpkg$(INSTALL_DIR)/bin/";\
	if [ -d dpkg_man ];\
	then\
		mkdir -p "dpkg$(INSTALL_DIR)/share/man";\
		mv dpkg_man/* "dpkg$(INSTALL_DIR)/share/man/";\
		rm -fr dpkg_man;\
	fi;\
	if [ -d dpkg_etc ];\
	then\
		mkdir -p "dpkg/etc";\
		mv dpkg_etc/* "dpkg/etc/";\
		rm -fr dpkg_etc;\
	fi;\
	SIZE=`du -sb dpkg | awk '$$1~/^([0-9])/ { print $$1 }'`;\
	sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" $(CONTROL_TEMPLATE) >dpkg/DEBIAN/control.1;\
	sed -e "s/AND SIZE HERE/$$SIZE/" dpkg/DEBIAN/control.1 >dpkg/DEBIAN/control.2;\
	sed -e "s/VERSION GOES HERE/${VERSION}-${RELEASE}/" dpkg/DEBIAN/control.2 >dpkg/DEBIAN/control;\
	rm -f dpkg/DEBIAN/control.1 dpkg/DEBIAN/control.2;\
	echo "Building package $(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb";\
	dpkg -b dpkg "$(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb" >/dev/null;\
	rm -fr dpkg;



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
	   cp -rH * /tmp/$$NM; \
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
