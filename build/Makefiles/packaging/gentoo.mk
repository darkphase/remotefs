
#############################
# remove all temporary objects
#############################

clean_gentoo_tmp: dummy
clean_gentoo: dummy clean_gentoo_tmp
	$(RM) -f *.ebuild

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
