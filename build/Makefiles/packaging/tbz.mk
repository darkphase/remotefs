
TEMP_TBZ = "remotefs.tar.bz2"

#############################
# remove all temporary objects
#############################

clean_tbz_tmp: dummy
	$(RM) -fr remotefs-*/
	$(RM) -f "$(TEMP_TBZ)"

clean_tbz: dummy clean_tbz_tmp
	$(RM) -f remotefs-*.tar.bz2

#############################
# Build tarball
#############################

tbz: 	
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp
	echo "Building remotefs-$(VERSION)-$(RELEASE).tar.bz2"
	
	chmod 700 build/init.d/rfsd.*
	
	tar --exclude .svn -cjf "$(TEMP_TBZ)" \
	src rfs_nss build Makefile \
	README LICENSE AUTHORS CHANGELOG
	
	mkdir -p "remotefs-$(VERSION)-$(RELEASE)"/
	tar -xjf "$(TEMP_TBZ)" -C "remotefs-$(VERSION)-$(RELEASE)"/
	tar --exclude *.tar.gz -cjf "remotefs-$(VERSION)-$(RELEASE).tar.bz2" "remotefs-$(VERSION)-$(RELEASE)"/
	
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp
