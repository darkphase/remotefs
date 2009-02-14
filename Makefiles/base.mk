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

rfsd: dummy
	@echo
	@$(MAKE) -f Makefiles/rfsd.mk flags build
	@echo

rfspasswd: dummy
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
	
	# ipkg
	$(RM) -fr ipkg/
	
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
	$(RM) -f *.ipk
	$(RM) -f *.ebuild

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

rfs_man:
	mkdir -p man/gz/man1
	gzip -c < man/rfs.1 > man/gz/man1/rfs.1.gz

rfsd_man:
	mkdir -p man/gz/man8
	gzip -c < man/rfsd.8 > man/gz/man8/rfsd.8.gz
	gzip -c < man/rfspasswd.8 > man/gz/man8/rfspasswd.8.gz

man: dummy rfs_man rfsd_man

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
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/kamikaze"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/gentoo"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/etc"
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)/init.d"
	cp debian/* "remotefs-$(VERSION)-$(RELEASE)/debian/"
	cp redhat/* "remotefs-$(VERSION)-$(RELEASE)/redhat/"
	cp kamikaze/* "remotefs-$(VERSION)-$(RELEASE)/kamikaze/"
	cp gentoo/* "remotefs-$(VERSION)-$(RELEASE)/gentoo/"
	cp etc/* "remotefs-$(VERSION)-$(RELEASE)/etc/"
	cp init.d/* "remotefs-$(VERSION)-$(RELEASE)/init.d/"
	cp Makefile "remotefs-$(VERSION)-$(RELEASE)/"
	chmod 700 remotefs-$(VERSION)-$(RELEASE)/init.d/*.*
	tar cf - remotefs-${VERSION}-${RELEASE} | bzip2 -c > remotefs-${VERSION}-${RELEASE}.tar.bz2
	
	$(MAKE) -sf Makefiles/base.mk clean_tmp

install_man: dummy
	mkdir -p $(INSTALL_DIR)/share/man;
	@-INSTALL_DIR=$(INSTALL_DIR) FILES="man/gz/*"; \
	for GZ_FILE in "$$FILES"; \
	do \
		cp -r $$GZ_FILE $$INSTALL_DIR/share/man; \
	done

uninstall_man: dummy
	@-INSTALL_DIR=$(INSTALL_DIR) FILES="man/gz/*"; \
	for GZ_FILE in "$$FILES"; \
	do \
		rm -f $$INSTALL_DIR/share/man/$$GZ_FILE; \
	done

#############################
# Build deb packages
#############################

debrelease: rfsdeb rfsddeb

debbase:
	mkdir -p "dpkg$(INSTALL_DIR)";
	mkdir -p "dpkg$(INSTALL_DIR)/bin";
	mkdir -p "dpkg/DEBIAN";

rfsmanpages: dummy
	$(MAKE) -sf Makefiles/base.mk man
	mkdir -p dpkg_man/man1/
	cp man/gz/man1/rfs.1.gz dpkg_man/man1/

rfsdeb: dummy clean_tmp debbase rfsmanpages
	echo "Building package rfs_$(VERSION)-$(RELEASE)_$(ARCH).deb"
	mkdir -p "dpkg$(INSTALL_DIR)/lib";
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
	mkdir -p "dpkg_etc/init.d/";
	cp etc/rfs-exports "dpkg_etc/";
	chmod 600 "dpkg_etc/rfs-exports";
	cp init.d/rfsd.debian "dpkg_etc/init.d/rfsd";
	chmod +x "dpkg_etc/init.d/rfsd"

rfsddeb: dummy clean_tmp debbase rfsdmanpages rfsdetc
	echo "Building package rfsd_$(VERSION)-$(RELEASE)_$(ARCH).deb"
	$(MAKE) -f Makefiles/base.mk clean_build
	$(MAKE) -f Makefiles/base.mk rfspasswd >/dev/null
	$(MAKE) -f Makefiles/base.mk clean_build
	$(MAKE) -f Makefiles/base.mk rfsd >/dev/null
	cp rfsd "dpkg$(INSTALL_DIR)/bin/";
	cp rfspasswd "dpkg$(INSTALL_DIR)/bin/";
	cp debian/conffiles dpkg/DEBIAN/
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
	fakeroot chown -R 0:0 dpkg/;
	fakeroot dpkg -b dpkg "$(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb" >/dev/null;
	$(MAKE) -f Makefiles/base.mk clean_bins
	$(MAKE) -f Makefiles/base.mk clean_packages_tmp

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
	$(MAKE) -f Makefiles/base.mk clean_packages_tmp

#############################
# Build ipkg
#############################

ipk: ipk-rfsd

ipkbase: dummy
	mkdir -p "ipkg/$(IPKNAME)/CONTROL/";
	mkdir -p "ipkg/$(IPKNAME)$(INSTALL_DIR)/bin";

rfsdipk: dummy ipkbase
	if [ -z "$(EXPERIMENTAL)" ]; then \
	    echo "Building package rfsd_$(VERSION)-$(RELEASE)_$(ARCH)_experimental.ipk"; \
	else \
	    echo "Building package rfsd_$(VERSION)-$(RELEASE)_$(ARCH).ipk"; \
	fi
	
	$(MAKE) -sf Makefiles/base.mk clean_tmp;
	IPKNAME=rfsd $(MAKE) -f Makefiles/base.mk ipkbase;
	mkdir -p "ipkg/rfsd/etc/init.d";
	$(MAKE) -f Makefiles/base.mk clean_build;
	$(MAKE) -f Makefiles/base.mk rfspasswd >/dev/null;
	$(MAKE) -f Makefiles/base.mk clean_build;
	$(MAKE) -f Makefiles/base.mk rfsd >/dev/null;
	cp rfsd "ipkg/rfsd$(INSTALL_DIR)/bin/";
	cp rfspasswd "ipkg/rfsd$(INSTALL_DIR)/bin/";
	cp init.d/rfsd.kamikaze "ipkg/rfsd/etc/init.d/rfsd";
	chmod +x "ipkg/rfsd/etc/init.d/rfsd";
	cp etc/rfs-exports "ipkg/rfsd/etc/";
	chmod 600 "ipkg/rfsd/etc/rfs-exports";
	cp kamikaze/conffiles "ipkg/rfsd/CONTROL/";
	IPKNAME=rfsd $(MAKE) -f Makefiles/base.mk buildipk;
	$(MAKE) -sf Makefiles/base.mk clean_tmp;
	
buildipk: dummy
	sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" \
	-e "s/VERSION GOES HERE/${VERSION}-${RELEASE}/" \
	"kamikaze/control.$(IPKNAME)" >"ipkg/$(IPKNAME)/CONTROL/control";
	fakeroot chown -R 0:0 "ipkg/$(IPKNAME)";
	fakeroot ipkg-build -c "ipkg/$(IPKNAME)" . 2>&1 >/dev/null;
	
	if [ -z "$(EXPERIMENTAL)" ]; then \
	    mv "$(IPKNAME)_$(VERSION)-$(RELEASE)_$(ARCH).ipk" "$(IPKNAME)_$(VERSION)-$(RELEASE)_$(ARCH)_experimental.ipk";\
	fi
	
	$(MAKE) -f Makefiles/base.mk clean_bins;
	$(MAKE) -f Makefiles/base.mk clean_packages_tmp;

#############################
# Gentoo ebuilds
#############################

rfsdebuild: dummy
	TARGET="rfsd" $(MAKE) -f Makefiles/base.mk genebuild
    
rfsebuild: dummy
	TARGET="rfs" $(MAKE) -f Makefiles/base.mk genebuild

genebuild: dummy
	echo "Creating $(TARGET)-${VERSION}-r${RELEASE}.ebuild"
	sed -e "s/INSERT BUILDDIR HERE/\"remotefs-${VERSION}-${RELEASE}\"/" \
	-e "s/VERSION HERE/${VERSION}-${RELEASE}/" \
	"gentoo/$(TARGET).ebuild" > "$(TARGET)-${VERSION}-r${RELEASE}.ebuild";

dummy:

#############################
# Dependencies for all proj.
#############################

include Makefiles/depends.mk
