
#############################
# remove all temporary objects
#############################

clean_tbz_tmp: dummy
	$(RM) -fr remotefs-*/

clean_tbz: dummy clean_tbz_tmp
	$(RM) -f remotefs-${VERSION}-${RELEASE}.tar.bz2

#############################
# Build tarball
#############################

tbz: 	
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp
	echo "Building remotefs-$(VERSION)-$(RELEASE).tar.bz2"
	chmod 700 build/init.d/rfsd.*
	
	tar --exclude .svn -cjf "remotefs-$(VERSION)-$(RELEASE).tar.bz2" \
	src rfs_nss build Makefile custom.mk \
	README LICENSE AUTHORS CHANGELOG
	
	$(MAKE) -sf build/Makefiles/base.mk clean_tmp
