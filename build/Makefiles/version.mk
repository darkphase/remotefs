VER_MAJOR=1
VER_MINOR=1
VERSION="$(VER_MAJOR).$(VER_MINOR)"
RELEASE=1
VERSION_FILE="src/version.h"

include build/Makefiles/install.mk

force_version:
	echo "/* Automatically generated */" > "$(VERSION_FILE)"
	echo "" >> "$(VERSION_FILE)"
	echo "#ifndef RFS_VERSION_H" >> "$(VERSION_FILE)"
	echo "#define RFS_VERSION_H" >> "$(VERSION_FILE)"
	echo "" >> "$(VERSION_FILE)"
	echo "#define RFS_VERSION_MAJOR $(VER_MAJOR)" >> "$(VERSION_FILE)"
	echo "#define RFS_VERSION_MINOR $(VER_MINOR)" >> "$(VERSION_FILE)"
	echo "#define RFS_VERSION $(VERSION)f" >> "$(VERSION_FILE)"
	echo "#define RFS_RELEASE $(RELEASE)" >> "$(VERSION_FILE)"
	echo "#define RFS_FULL_VERSION \"$(VERSION)-$(RELEASE)\"" >> "$(VERSION_FILE)"
	echo "" >> "$(VERSION_FILE)"
	echo "#endif /* RFS_VERSION_H */" >> "$(VERSION_FILE)"
	echo "" >> "$(VERSION_FILE)"

make_version:
	if [ ! -f "$(VERSION_FILE)" ]; then \
		$(MAKE) -f build/Makefiles/version.mk force_version; \
	fi
