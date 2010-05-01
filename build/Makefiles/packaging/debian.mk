
#############################
# remove all temporary objects
#############################

clean_debian_tmp: dummy
	$(RM) -fr build/man/gz
	$(RM) -fr dpkg/ dpkg_man/ dpkg_etc/ dpkg_sbin/

clean_debian: dummy clean_debian_tmp
	$(RM) -f *.deb

#############################
# Build deb packages
#############################

debbase:
	mkdir -p "dpkg$(INSTALL_DIR)/bin";
	mkdir -p "dpkg/DEBIAN";

rfsmanpages: dummy
	$(MAKE) -sf build/Makefiles/base.mk man
	mkdir -p dpkg_man/man1/
	mkdir -p dpkg_man/man8/
	cp build/man/gz/man1/rfs.1.gz dpkg_man/man1/
	cp build/man/gz/man8/mount.rfs.8.gz dpkg_man/man8/

rfsdeb: dummy clean_build clean_debian_tmp debbase rfsmanpages
	echo "Building package rfs_$(VERSION)-$(RELEASE)_$(ARCH).deb"
	mkdir -p "dpkg$(INSTALL_DIR)/lib"
	mkdir -p "dpkg/sbin"
	cp build/sbin/mount.rfs "dpkg/sbin/"
	cp build/sbin/umount.fuse.rfs "dpkg/sbin/"
	$(MAKE) -f build/Makefiles/base.mk rfs >$(OUTPUT)
	cp rfs "dpkg$(INSTALL_DIR)/bin/"
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
	mkdir -p "dpkg_etc/init.d/"
	mkdir -p "dpkg_etc/default/"
	cp build/etc/rfs-exports "dpkg_etc/"
	chmod 600 "dpkg_etc/rfs-exports"
	cp build/init.d/rfsd.debian "dpkg_etc/init.d/rfsd"
	cp build/debian/default/rfsd "dpkg_etc/default/rfsd"
	chmod +x "dpkg_etc/init.d/rfsd"

rfsddeb: dummy clean_build clean_tmp debbase rfsdmanpages rfsdetc
	echo "Building package rfsd_$(VERSION)-$(RELEASE)_$(ARCH).deb"
	$(MAKE) -f build/Makefiles/base.mk clean_build
	$(MAKE) -f build/Makefiles/base.mk rfspasswd >$(OUTPUT)
	$(MAKE) -f build/Makefiles/base.mk clean_build
	$(MAKE) -f build/Makefiles/base.mk rfsd >$(OUTPUT)
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
	$(MAKE) -f build/Makefiles/base.mk librfs libnss nss >$(OUTPUT)
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
	fakeroot dpkg -b dpkg "$(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb" >$(OUTPUT)
	$(MAKE) -f build/Makefiles/base.mk clean_bins clean_packages_tmp
