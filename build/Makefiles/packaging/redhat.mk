#############################
# remove all temporary objects
#############################

clean_redhat_tmp: dummy
	$(RM) -fr rpmbuild/
	$(RM) -f .rpmmacros
	$(RM) -fr rfsd-*/
	$(RM) -fr rfs-*/

clean_redhat: dummy clean_redhat_tmp
	$(RM) -f *.rpm

#############################
# Build RPM
#############################

rfsrpm: dummy
	$(MAKE) -f build/Makefiles/base.mk clean_tmp
	$(MAKE) -f build/Makefiles/base.mk man
	RPMNAME=rfs $(MAKE) -f build/Makefiles/base.mk buildrpm
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
	HOME=`pwd`/rpmbuild rpmbuild -bb --target $(ARCH) rpmbuild/SPECS/$(RPMNAME).spec >$(OUTPUT) 2>&1
	cp rpmbuild/RPMS/$(RPMNAME)-$(VERSION)-$(RELEASE).${ARCH}.rpm .
	$(MAKE) -f build/Makefiles/base.mk clean_bins
	$(MAKE) -f build/Makefiles/base.mk clean_packages_tmp
