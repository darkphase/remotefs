
#############################
# remove all temporary objects
#############################

clean_kamikaze_tmp: dummy
	$(RM) -fr ipkg/

clean_kamikaze: dummy clean_kamikaze_tmp
	$(RM) -f *.ipk

#############################
# Build ipkg
#############################

ipkbase: dummy
	mkdir -p "ipkg/$(IPKNAME)/CONTROL/";
	mkdir -p "ipkg/$(IPKNAME)$(INSTALL_DIR)/bin";

rfsdipk: dummy ipkbase
	if [ -z "$(EXPERIMENTAL)" ]; then \
	    echo "Building package rfsd_$(VERSION)-$(RELEASE)_$(ARCH)_experimental.ipk"; \
	else \
	    echo "Building package rfsd_$(VERSION)-$(RELEASE)_$(ARCH).ipk"; \
	fi
	
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp
	IPKNAME=rfsd $(MAKE) -f build/Makefiles/base.mk ipkbase
	mkdir -p "ipkg/rfsd/etc/init.d"
	$(MAKE) -f build/Makefiles/base.mk clean_build
	$(MAKE) -f build/Makefiles/base.mk rfspasswd >$(OUTPUT)
	$(MAKE) -f build/Makefiles/base.mk clean_build
	$(MAKE) -f build/Makefiles/base.mk rfsd >$(OUTPUT)
	cp rfsd "ipkg/rfsd$(INSTALL_DIR)/bin/"
	cp rfspasswd "ipkg/rfsd$(INSTALL_DIR)/bin/"
	cp build/init.d/rfsd.kamikaze "ipkg/rfsd/etc/init.d/rfsd"
	chmod +x "ipkg/rfsd/etc/init.d/rfsd"
	cp build/etc/rfs-exports "ipkg/rfsd/etc/"
	chmod 600 "ipkg/rfsd/etc/rfs-exports"
	cp build/kamikaze/conffiles "ipkg/rfsd/CONTROL/"
	chmod 644 "ipkg/rfsd/CONTROL/conffiles"
	IPKNAME=rfsd $(MAKE) -f build/Makefiles/base.mk buildipk
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp
	
buildipk: dummy
	sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" \
	-e "s/VERSION GOES HERE/${VERSION}-${RELEASE}/" \
	"build/kamikaze/control.$(IPKNAME)" >"ipkg/$(IPKNAME)/CONTROL/control"
	fakeroot chown -R 0:0 "ipkg/$(IPKNAME)"
	fakeroot ipkg-build -c "ipkg/$(IPKNAME)" . >$(OUTPUT)
	
	if [ -z "$(EXPERIMENTAL)" ]; then \
	    mv "$(IPKNAME)_$(VERSION)-$(RELEASE)_$(ARCH).ipk" "$(IPKNAME)_$(VERSION)-$(RELEASE)_$(ARCH)_experimental.ipk";\
	fi
	
	$(MAKE) -f build/Makefiles/base.mk clean_bins
	$(MAKE) -f build/Makefiles/base.mk clean_packages_tmp
