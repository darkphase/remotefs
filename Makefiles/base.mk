##############################
# Get platform dependent flags
##############################
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname

include Makefiles/$(OS)$(ALT).mk
include Makefiles/options.mk
include Makefiles/version.mk

librfs: dummy
	@echo
	@$(MAKE) -f Makefiles/librfs.mk flags build
	@echo

rfs: dummy librfs
	@echo
	@$(MAKE) -f Makefiles/rfs.mk flags build
	@echo

librfsd: dummy
	@echo
	@$(MAKE) -f Makefiles/librfsd.mk flags build
	@echo

rfsd: dummy librfsd
	@echo
	@$(MAKE) -f Makefiles/rfsd.mk flags build
	@echo

rfspasswd: dummy librfsd
	@echo
	@$(MAKE) -f Makefiles/rfspasswd.mk flags build
	@echo

#############################
# remove all temporaries objects
#############################

clean_build: dummy
	$(RM) -f src/*.o
	$(RM) -f src/md5crypt/*.o

clean_bins: dummy
	$(RM) -f rfs
	$(RM) -f rfsd
	$(RM) -f rfspasswd
	$(RM) -f *.so*

clean_packages_tmp: dummy
	# debs
	$(RM) -fr man/gz
	$(RM) -fr dpkg/ dpkg_man/ dpkg_etc/
	
	# rpms
	$(RM) -fr rpmbuild/
	$(RM) -f .rpmmacros
	$(RM) -fr rfsd-*/
	$(RM) -fr rfs-*/
	
	# tbz
	$(RM) -fr remotefs-*/
	
clean_tmp: dummy clean_packages_tmp clean_build

clean_packages: dummy clean_packages_tmp
	$(RM) -f remotefs-${VERSION}-${RELEASE}.tar.bz2
	$(RM) -f *.deb
	$(RM) -f *.rpm

clean: clean_build clean_bins clean_packages

#############################
# Rebuild dependency file
#############################
depends:
	@grep 'include *".*"' src/*.c | sed -e 's/\.c/.o/' -e 's/# *include *"\(.*\.[ch]\)"/src\/\1/' > Makefiles/depends.mk
	@ls src/*.c | sed -e 's/\([^\.]*\)/\1.o:\1/' >> Makefiles/depends.mk

#######################################
# Rules for packaging, ...
#######################################

#############################
# Build man pages
#############################

man: dummy
	mkdir -p man/gz/man1
	mkdir -p man/gz/man8
	gzip -c < man/rfs.1 > man/gz/man1/rfs.1.gz
	gzip -c < man/rfsd.8 > man/gz/man8/rfsd.8.gz
	gzip -c < man/rfspasswd.8 > man/gz/man8/rfspasswd.8.gz

#############################
# Build tarball
#############################

tbz: 	
	$(MAKE) -sf Makefiles/base.mk clean_tmp
	
	echo "Building remotefs-${VERSION}-${RELEASE}.tar.bz2"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/src/md5crypt/"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/src/acl/doc/"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/src/acl/include/"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/src/acl/libacl/"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/man/"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/Makefiles/"
	cp LICENSE "remotefs-$(VERSION)-$(RELEASE)/"
	cp AUTHORS "remotefs-$(VERSION)-$(RELEASE)/"
	cp CHANGELOG "remotefs-$(VERSION)-$(RELEASE)/"
	cp src/*.c src/*.h "remotefs-$(VERSION)-$(RELEASE)/src/"
	cp src/md5crypt/*.c src/md5crypt/*.h "remotefs-$(VERSION)-$(RELEASE)/src/md5crypt/"
	cp src/md5crypt/LSM src/md5crypt/ORIGIN src/md5crypt/README "remotefs-$(VERSION)-$(RELEASE)/src/md5crypt/"
	cp src/acl/include/*.h "remotefs-$(VERSION)-$(RELEASE)/src/acl/include/"
	cp src/acl/doc/* "remotefs-$(VERSION)-$(RELEASE)/src/acl/doc/"
	cp src/acl/libacl/*.h "remotefs-$(VERSION)-$(RELEASE)/src/acl/libacl/"
	cp src/acl/ORIGIN "remotefs-$(VERSION)-$(RELEASE)/src/acl/"
	cp man/*.1 man/*.8 "remotefs-$(VERSION)-$(RELEASE)/man/"
	cp Makefiles/* "remotefs-$(VERSION)-$(RELEASE)/Makefiles/"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/debian"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/redhat"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/etc"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/init.d"
	cp debian/* "remotefs-$(VERSION)-$(RELEASE)/debian/"
	cp redhat/* "remotefs-$(VERSION)-$(RELEASE)/redhat/"
	cp etc/* "remotefs-$(VERSION)-$(RELEASE)/etc/"
	cp init.d/* "remotefs-$(VERSION)-$(RELEASE)/init.d/"
	cp Makefile "remotefs-$(VERSION)-$(RELEASE)/"
	chmod 700 remotefs-$(VERSION)-$(RELEASE)/init.d/*.*
	tar cf - remotefs-${VERSION}-${RELEASE} | bzip2 -c > remotefs-${VERSION}-${RELEASE}.tar.bz2
	
	$(MAKE) -sf Makefiles/base.mk clean_tmp

install_man:
	@-INSTALL_DIR=$(INSTALL_DIR) FILES="man/gz/*"; \
	for GZ_FILE in "$$FILES"; \
	do \
		cp -r $$GZ_FILE $$INSTALL_DIR/share/man; \
	done

install: install_man
	@TARGET_DIR=$(INSTALL_DIR)/lib $(MAKE) -sf Makefiles/librfs.mk install_librfs
	@TARGET_DIR=$(INSTALL_DIR)/bin $(MAKE) -sf Makefiles/rfs.mk install_rfs
	@TARGET_DIR=$(INSTALL_DIR)/lib $(MAKE) -sf Makefiles/librfsd.mk install_librfsd
	@TARGET_DIR=$(INSTALL_DIR)/bin $(MAKE) -sf Makefiles/rfsd.mk install_rfsd
	@TARGET_DIR=$(INSTALL_DIR)/bin $(MAKE) -sf Makefiles/rfspasswd.mk install_rfspasswd

#############################
# Build deb packages
#############################

debrelease: rfsdeb rfsddeb

debbase:
	mkdir -p "dpkg$(INSTALL_DIR)";
	mkdir -p "dpkg$(INSTALL_DIR)/bin";
	mkdir -p "dpkg$(INSTALL_DIR)/lib";
	mkdir -p "dpkg/DEBIAN";

rfsmanpages: dummy
	$(MAKE) -sf Makefiles/base.mk man
	mkdir -p dpkg_man/man1/
	cp man/gz/man1/rfs.1.gz dpkg_man/man1/

rfsdeb: dummy clean_tmp debbase rfsmanpages
	echo "Building package rfs_$(VERSION)-$(RELEASE)_$(ARCH).deb"
	$(MAKE) -f Makefiles/base.mk rfs >/dev/null
	cp rfs "dpkg$(INSTALL_DIR)/bin/";
	cp librfs.$(SO_EXT).$(VERSION) "dpkg$(INSTALL_DIR)/lib/"
	ln -sf "librfs.$(SO_EXT).$(VERSION)" "dpkg$(INSTALL_DIR)/lib/librfs.$(SO_EXT)"
	CONTROL_TEMPLATE="debian/control.rfs" \
	NAME="rfs" \
	$(MAKE) -f Makefiles/base.mk builddeb

rfsdmanpages: dummy
	$(MAKE) -sf Makefiles/base.mk man
	mkdir -p dpkg_man/man8/
	cp man/gz/man8/rfsd.8.gz dpkg_man/man8/
	cp man/gz/man8/rfspasswd.8.gz dpkg_man/man8/

rfsdetc: dummy
	mkdir -p "dpkg_etc/init.d/";\
	cp etc/rfs-exports "dpkg_etc/";\
	cp init.d/rfsd.debian "dpkg_etc/init.d/rfsd";\
	chmod +x "dpkg_etc/init.d/rfsd"

rfsddeb: dummy clean_tmp debbase rfsdmanpages rfsdetc
	echo "Building package rfsd_$(VERSION)-$(RELEASE)_$(ARCH).deb"
	$(MAKE) -f Makefiles/base.mk clean_build
	$(MAKE) -f Makefiles/base.mk librfsd >/dev/null
	$(MAKE) -f Makefiles/base.mk clean_build
	$(MAKE) -f Makefiles/base.mk rfspasswd >/dev/null
	$(MAKE) -f Makefiles/base.mk clean_build
	$(MAKE) -f Makefiles/base.mk rfsd >/dev/null
	cp rfsd "dpkg$(INSTALL_DIR)/bin/";
	cp rfspasswd "dpkg$(INSTALL_DIR)/bin/";
	cp librfsd.$(SO_EXT).$(VERSION) "dpkg$(INSTALL_DIR)/lib/"
	cp debian/conffiles dpkg/DEBIAN/
	ln -sf "librfsd.$(SO_EXT).$(VERSION)" "dpkg$(INSTALL_DIR)/lib/librfsd.$(SO_EXT)"
	CONTROL_TEMPLATE="debian/control.rfsd" \
	NAME="rfsd" \
	$(MAKE) -f Makefiles/base.mk builddeb

builddeb: dummy
	if [ -d dpkg_man ];\
	then\
		mkdir -p "dpkg$(INSTALL_DIR)/share/man";\
		mv dpkg_man/* "dpkg$(INSTALL_DIR)/share/man/";\
		rm -fr dpkg_man;\
	fi;
	if [ -d dpkg_etc ];\
	then\
		mkdir -p "dpkg/etc";\
		mv dpkg_etc/* "dpkg/etc/";\
		rm -fr dpkg_etc;\
	fi;
	sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" \
	-e "s/AND SIZE HERE/`du -sk dpkg | awk '$$1~/^([0-9])/ { print $$1 }'`/" \
	-e "s/VERSION GOES HERE/${VERSION}-${RELEASE}/" \
	$(CONTROL_TEMPLATE) >dpkg/DEBIAN/control
	dpkg -b dpkg "$(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb" >/dev/null;
	$(MAKE) -sf Makefiles/base.mk clean_bins

#############################
# Build RPM
#############################

rpm: rpm-rfsd rpm-rfs

rfsrpm: dummy
	$(MAKE) -sf Makefiles/base.mk clean_tmp
	$(MAKE) -f Makefiles/base.mk man
	RPMNAME=rfs $(MAKE) -sf Makefiles/base.mk buildrpm
	$(MAKE) -sf Makefiles/base.mk clean_tmp
	
rfsdrpm: dummy
	$(MAKE) -sf Makefiles/base.mk clean_tmp
	$(MAKE) -f Makefiles/base.mk man
	RPMNAME=rfsd $(MAKE) -f Makefiles/base.mk buildrpm
	$(MAKE) -sf Makefiles/base.mk clean_tmp

redhat/%.spec: dummy Makefiles/version.mk
	sed -e "s/Version:.*/Version:$(VERSION)/"  \
	-e "s/Release:.*/Release:$(RELEASE)/" $@ \
	> rpmbuild/SPECS/$(RPMNAME).spec

rpmbuild: dummy
	PWD=`pwd`
	mkdir -p rpmbuild/BUILD rpmbuild/SOURCES rpmbuild/RPMS rpmbuild/SPECS
	echo '%_topdir %(echo $(PWD)/rpmbuild)' > rpmbuild/.rpmmacros
	echo '%_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm' >> rpmbuild/.rpmmacros
	echo '%debug_package %{nil}' >> rpmbuild/.rpmmacros

buildrpm: rpmbuild redhat/$(RPMNAME).spec
	mkdir -p $(RPMNAME)-$(VERSION)/src
	mkdir -p $(RPMNAME)-$(VERSION)/Makefiles
	mkdir -p $(RPMNAME)-$(VERSION)/init.d
	mkdir -p $(RPMNAME)-$(VERSION)/etc
	mkdir -p $(RPMNAME)-$(VERSION)/man
	cp -r src/* $(RPMNAME)-$(VERSION)/src
	cp Makefiles/* $(RPMNAME)-$(VERSION)/Makefiles/
	cp init.d/* $(RPMNAME)-$(VERSION)/init.d/
	cp etc/* $(RPMNAME)-$(VERSION)/etc/
	cp -r man/gz/* $(RPMNAME)-$(VERSION)/man/
	cp Makefile $(RPMNAME)-$(VERSION)/
	chmod 700 $(RPMNAME)-$(VERSION)/init.d/*
	tar -cpzf rpmbuild/SOURCES/$(RPMNAME)-$(VERSION).tar.gz $(RPMNAME)-$(VERSION)
	echo "Building package $(RPMNAME)-$(VERSION)-$(RELEASE).${ARCH}.rpm"
	HOME=`pwd`/rpmbuild rpmbuild -bb --target $(ARCH) rpmbuild/SPECS/$(RPMNAME).spec >/dev/null 2>&1
	cp rpmbuild/RPMS/$(RPMNAME)-$(VERSION)-$(RELEASE).${ARCH}.rpm .
	$(MAKE) -f Makefiles/base.mk clean_bins

dummy:

#############################
# Dependencies for all proj.
#############################

include Makefiles/depends.mk
