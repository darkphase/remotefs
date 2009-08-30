##############################
# Get platform dependent flags
##############################
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname

include build/Makefiles/$(OS)$(ALT).mk
include build/Makefiles/options.mk
include build/Makefiles/version.mk

libnss: dummy
	@echo
	@$(MAKE) -f build/Makefiles/libnss.mk flags build
	@echo

nss: dummy
	@echo
	@$(MAKE) -f build/Makefiles/nssd.mk flags build
	@echo

librfs: dummy
	@echo
	@$(MAKE) -f build/Makefiles/librfs.mk flags build
	@echo

rfs: dummy librfs
	@echo
	@$(MAKE) -f build/Makefiles/rfs.mk flags build
	@echo

rfsd: dummy
	@echo
	@$(MAKE) -f build/Makefiles/rfsd.mk flags build
	@echo

rfspasswd: dummy
	@echo
	@$(MAKE) -f build/Makefiles/rfspasswd.mk flags build
	@echo

#############################
# remove all temporaries objects
#############################

clean_build: dummy
	$(RM) -f src/*.o
	$(RM) -f src/md5crypt/*.o
	$(RM) -f rfs_nss/src/*.o

clean_bins: dummy
	$(RM) -f rfs
	$(RM) -f rfsd
	$(RM) -f rfspasswd
	$(RM) -f rfs_nssd
	$(RM) -f *.so*

clean_packages_tmp: dummy
	# debs
	$(RM) -fr build/man/gz
	$(RM) -fr dpkg/ dpkg_man/ dpkg_etc/ dpkg_sbin/
	
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

clean_version: 
	$(RM) -f src/version.h

clean_packages: dummy clean_packages_tmp
	$(RM) -f remotefs-${VERSION}-${RELEASE}.tar.bz2
	$(RM) -f *.deb
	$(RM) -f *.rpm
	$(RM) -f *.ipk
	$(RM) -f *.ebuild

clean: clean_build clean_bins clean_packages clean_version

#############################
# Rebuild dependency file
#############################
depends:
	@touch build/Makefiles/depends.mk
	@grep -E '#\s*include[^"]+"[^"]+"' src/*.c | sed -r -e 's/\.c/.o/' -e 's/#\s*include[^"]+"([^"]+)".*/src\/\1/' > build/Makefiles/depends.mk
	@grep -E '#\s*include[^"]+"[^"]+"' rfs_nss/src/*.c | sed -r -e 's/\.c/.o/' -e 's/#\s*include[^"]+"([^"]+)".*/rfs_nss\/src\/\1/' >> build/Makefiles/depends.mk
	@ls src/*.c | sed -e 's/\([^\.]*\)/\1.o:\1/' >> build/Makefiles/depends.mk
	@ls rfs_nss/src/*.c | sed -e 's/\([^\.]*\)/\1.o:\1/' >> build/Makefiles/depends.mk

#######################################
# Rules for packaging, ...
#######################################

#############################
# Build man pages
#############################

rfs_man:
	mkdir -p build/man/gz/man1
	gzip -c < build/man/rfs.1 > build/man/gz/man1/rfs.1.gz

rfsd_man:
	mkdir -p build/man/gz/man8
	gzip -c < build/man/rfsd.8 > build/man/gz/man8/rfsd.8.gz
	gzip -c < build/man/rfspasswd.8 > build/man/gz/man8/rfspasswd.8.gz

rfsnss_man: dummy
	mkdir -p build/man/gz/man1
	gzip -c < build/man/rfs_nssd.1 > build/man/gz/man1/rfs_nssd.1.gz
	gzip -c < build/man/rfsnsswitch.sh.1 > build/man/gz/man1/rfsnsswitch.sh.1.gz

man: dummy rfs_man rfsd_man rfsnss_man

#############################
# Build tarball
#############################

tbz: 	
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp
	echo "Building remotefs-$(VERSION)-$(RELEASE).tar.bz2"
	chmod 700 build/init.d/rfsd.*
	tar --exclude .svn -cjf "remotefs-$(VERSION)-$(RELEASE).tar.bz2" src rfs_nss build Makefile README LICENSE AUTHORS CHANGELOG
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp

install_man: dummy
	mkdir -p $(INSTALL_DIR)/share/man;
	@-INSTALL_DIR=$(INSTALL_DIR) FILES="build/man/gz/*"; \
	for GZ_FILE in "$$FILES"; \
	do \
		cp -r $$GZ_FILE $$INSTALL_DIR/share/man; \
	done

uninstall_man: dummy
	@-INSTALL_DIR=$(INSTALL_DIR) FILES="build/man/gz/*"; \
	for GZ_FILE in "$$FILES"; \
	do \
		rm -f $$INSTALL_DIR/share/man/$$GZ_FILE; \
	done

#############################
# Build deb packages
#############################

debbase:
	mkdir -p "dpkg$(INSTALL_DIR)/bin";
	mkdir -p "dpkg/DEBIAN";

rfsmanpages: dummy
	$(MAKE) -sf build/Makefiles/base.mk man
	mkdir -p dpkg_man/man1/
	cp build/man/gz/man1/rfs.1.gz dpkg_man/man1/

rfsdeb: dummy clean_tmp debbase rfsmanpages
	echo "Building package rfs_$(VERSION)-$(RELEASE)_$(ARCH).deb"
	mkdir -p "dpkg$(INSTALL_DIR)/lib";
	$(MAKE) -f build/Makefiles/base.mk rfs >/dev/null
	cp rfs "dpkg$(INSTALL_DIR)/bin/";
	cp librfs.$(SO_EXT).$(VERSION) "dpkg$(INSTALL_DIR)/lib/"
	ln -sf "librfs.$(SO_EXT).$(VERSION)" "dpkg$(INSTALL_DIR)/lib/librfs.$(SO_EXT)"
	CONTROL_TEMPLATE="build/debian/control.rfs" \
	NAME="rfs" \
	$(MAKE) -f build/Makefiles/base.mk builddeb

rfsdmanpages: dummy
	$(MAKE) -sf build/Makefiles/base.mk man
	mkdir -p dpkg_man/man8/
	cp build/man/gz/man8/rfsd.8.gz dpkg_man/man8/
	cp build/man/gz/man8/rfspasswd.8.gz dpkg_man/man8/

rfsdetc: dummy
	mkdir -p "dpkg_etc/init.d/";
	cp build/etc/rfs-exports "dpkg_etc/";
	chmod 600 "dpkg_etc/rfs-exports";
	cp build/init.d/rfsd.debian "dpkg_etc/init.d/rfsd";
	chmod +x "dpkg_etc/init.d/rfsd"

rfsddeb: dummy clean_tmp debbase rfsdmanpages rfsdetc
	echo "Building package rfsd_$(VERSION)-$(RELEASE)_$(ARCH).deb"
	$(MAKE) -f build/Makefiles/base.mk clean_build
	$(MAKE) -f build/Makefiles/base.mk rfspasswd >/dev/null
	$(MAKE) -f build/Makefiles/base.mk clean_build
	$(MAKE) -f build/Makefiles/base.mk rfsd >/dev/null
	cp rfsd "dpkg$(INSTALL_DIR)/bin/";
	cp rfspasswd "dpkg$(INSTALL_DIR)/bin/";
	cp build/debian/conffiles dpkg/DEBIAN/
	CONTROL_TEMPLATE="build/debian/control.rfsd" \
	NAME="rfsd" \
	$(MAKE) -f build/Makefiles/base.mk builddeb

rfsnssmanpages: dummy
	$(MAKE) -sf build/Makefiles/base.mk rfsnss_man
	mkdir -p dpkg_man/man1/
	cp build/man/gz/man1/rfs_nssd.1.gz dpkg_man/man1/
	cp build/man/gz/man1/rfsnsswitch.sh.1.gz dpkg_man/man1/

rfsnsssbin: dummy
	mkdir -p "dpkg_sbin/"	
	cp build/sbin/rfsnsswitch.sh "dpkg_sbin/";

rfsnssdeb: dummy clean_tmp debbase rfsnssmanpages rfsnsssbin
	echo "Building package rfsnss_$(VERSION)-$(RELEASE)_$(ARCH).deb"
	cp build/debian/rfs_nss/post* build/debian/rfs_nss/pre* dpkg/DEBIAN/
	$(MAKE) -f build/Makefiles/base.mk clean_build
	$(MAKE) -f build/Makefiles/base.mk librfs >/dev/null
	$(MAKE) -f build/Makefiles/base.mk libnss >/dev/null
	$(MAKE) -f build/Makefiles/base.mk nss >/dev/null
	mkdir -p "dpkg/lib";
	cp rfs_nssd "dpkg$(INSTALL_DIR)/bin/";
	cp libnss_rfs.so.2 "dpkg/lib/";
	CONTROL_TEMPLATE="build/debian/control.rfsnss" \
	NAME="rfsnss" \
	$(MAKE) -f build/Makefiles/base.mk builddeb

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
	if [ -d dpkg_sbin ];\
	then\
		mkdir -p "dpkg$(INSTALL_DIR)/sbin/";\
		mv dpkg_sbin/* "dpkg$(INSTALL_DIR)/sbin/";\
		rm -fr dpkg_sbin;\
	fi;
	sed -e "s/INSERT ARCH HERE, PLEASE/$(ARCH)/" \
	-e "s/AND SIZE HERE/`du -sk dpkg | awk '$$1~/^([0-9])/ { print $$1 }'`/" \
	-e "s/VERSION GOES HERE/${VERSION}-${RELEASE}/" \
	$(CONTROL_TEMPLATE) >dpkg/DEBIAN/control
	fakeroot chown -R 0:0 dpkg/;
	fakeroot dpkg -b dpkg "$(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb" >/dev/null;
	$(MAKE) -f build/Makefiles/base.mk clean_bins
	$(MAKE) -f build/Makefiles/base.mk clean_packages_tmp

#############################
# Build RPM
#############################

rfsrpm: dummy
	$(MAKE) -f build/Makefiles/base.mk clean_tmp
	$(MAKE) -f build/Makefiles/base.mk man
	RPMNAME=rfs $(MAKE) -sf build/Makefiles/base.mk buildrpm
	$(MAKE) -f build/Makefiles/base.mk clean_tmp
	
rfsdrpm: dummy
	$(MAKE) -f build/Makefiles/base.mk clean_tmp
	$(MAKE) -f build/Makefiles/base.mk man
	RPMNAME=rfsd $(MAKE) -f build/Makefiles/base.mk buildrpm
	$(MAKE) -f build/Makefiles/base.mk clean_tmp

rfsnssrpm: dummy
	$(MAKE) -f build/Makefiles/base.mk clean_tmp
	$(MAKE) -f build/Makefiles/base.mk man
	RPMNAME=rfsnss $(MAKE) -f build/Makefiles/base.mk buildrpm
	$(MAKE) -f build/Makefiles/base.mk clean_tmp
	

build/redhat/%.spec: dummy build/Makefiles/version.mk
	sed -e "s/Version:.*/Version:$(VERSION)/"  \
	-e "s/Release:.*/Release:$(RELEASE)/" $@ \
	> rpmbuild/SPECS/$(RPMNAME).spec

rpmbuild: dummy
	PWD=`pwd`
	mkdir -p rpmbuild/BUILD rpmbuild/SOURCES rpmbuild/RPMS rpmbuild/SPECS
	echo '%_topdir %(echo $(PWD)/rpmbuild)' > rpmbuild/.rpmmacros
	echo '%_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm' >> rpmbuild/.rpmmacros
	echo '%debug_package %{nil}' >> rpmbuild/.rpmmacros

buildrpm: rpmbuild build/redhat/$(RPMNAME).spec
	echo "Building package $(RPMNAME)-$(VERSION)-$(RELEASE).${ARCH}.rpm"
	mkdir -p $(RPMNAME)-$(VERSION)/build/man/man1
	mkdir -p $(RPMNAME)-$(VERSION)/build/man/man8
	tar --exclude .svn -cf - src rfs_nss build/init.d build/etc build/sbin build/Makefiles Makefile | (cd $(RPMNAME)-$(VERSION); tar xf -)
	cp build/man/*.1 $(RPMNAME)-$(VERSION)/build/man/man1/
	cd $(RPMNAME)-$(VERSION)/build/man/man1/; gzip *
	cp build/man/*.8 $(RPMNAME)-$(VERSION)/build/man/man8/
	cd $(RPMNAME)-$(VERSION)/build/man/man8/; gzip *
	chmod 700 $(RPMNAME)-$(VERSION)/build/init.d/*
	tar -cpzf rpmbuild/SOURCES/$(RPMNAME)-$(VERSION).tar.gz $(RPMNAME)-$(VERSION)
	rm -fr $(RPMNAME)-$(VERSION)
	HOME=`pwd`/rpmbuild rpmbuild -bb --target $(ARCH) rpmbuild/SPECS/$(RPMNAME).spec >/dev/null 2>&1
	cp rpmbuild/RPMS/$(RPMNAME)-$(VERSION)-$(RELEASE).${ARCH}.rpm .
	$(MAKE) -f build/Makefiles/base.mk clean_bins
	$(MAKE) -f build/Makefiles/base.mk clean_packages_tmp
	

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
	
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp;
	IPKNAME=rfsd $(MAKE) -f build/Makefiles/base.mk ipkbase;
	mkdir -p "ipkg/rfsd/etc/init.d";
	$(MAKE) -f build/Makefiles/base.mk clean_build;
	$(MAKE) -f build/Makefiles/base.mk rfspasswd >/dev/null;
	$(MAKE) -f build/Makefiles/base.mk clean_build;
	$(MAKE) -f build/Makefiles/base.mk rfsd >/dev/null;
	cp rfsd "ipkg/rfsd$(INSTALL_DIR)/bin/";
	cp rfspasswd "ipkg/rfsd$(INSTALL_DIR)/bin/";
	cp build/init.d/rfsd.kamikaze "ipkg/rfsd/etc/init.d/rfsd";
	chmod +x "ipkg/rfsd/etc/init.d/rfsd";
	cp build/etc/rfs-exports "ipkg/rfsd/etc/";
	chmod 600 "ipkg/rfsd/etc/rfs-exports";
	cp build/kamikaze/conffiles "ipkg/rfsd/CONTROL/";
	IPKNAME=rfsd $(MAKE) -f build/Makefiles/base.mk buildipk;
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp;
	
buildipk: dummy
	sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" \
	-e "s/VERSION GOES HERE/${VERSION}-${RELEASE}/" \
	"build/kamikaze/control.$(IPKNAME)" >"ipkg/$(IPKNAME)/CONTROL/control";
	fakeroot chown -R 0:0 "ipkg/$(IPKNAME)";
	fakeroot ipkg-build -c "ipkg/$(IPKNAME)" . >/dev/null;
	
	if [ -z "$(EXPERIMENTAL)" ]; then \
	    mv "$(IPKNAME)_$(VERSION)-$(RELEASE)_$(ARCH).ipk" "$(IPKNAME)_$(VERSION)-$(RELEASE)_$(ARCH)_experimental.ipk";\
	fi
	
	$(MAKE) -f build/Makefiles/base.mk clean_bins;
	$(MAKE) -f build/Makefiles/base.mk clean_packages_tmp;

#############################
# Gentoo ebuilds
#############################

rfsdebuild: dummy
	TARGET="rfsd" $(MAKE) -f build/Makefiles/base.mk genebuild
    
rfsebuild: dummy
	TARGET="rfs" $(MAKE) -f build/Makefiles/base.mk genebuild

rfssslebuild: dummy
	TARGET="rfs-ssl" $(MAKE) -f build/Makefiles/base.mk genebuild

rfsnssebuild: dummy
	TARGET="rfsnss" $(MAKE) -f build/Makefiles/base.mk genebuild

genebuild: dummy
	echo "Creating $(TARGET)-${VERSION}-r${RELEASE}.ebuild"
	sed -e "s/INSERT BUILDDIR HERE/\"remotefs-${VERSION}-${RELEASE}\"/" \
	-e "s/VERSION HERE/${VERSION}-${RELEASE}/" \
	-e "s/JUST VERSION/${VERSION}/" \
	-e "s/GENTOO VERSION/${VERSION}-r${RELEASE}/" \
	"build/gentoo/$(TARGET).ebuild" > "$(TARGET)-${VERSION}-r${RELEASE}.ebuild";

dummy:

#############################
# Dependencies for all proj.
#############################

include build/Makefiles/depends.mk
