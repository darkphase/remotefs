VER_MAJOR=0
VER_MINOR=13
BUILDDATETIME=`date`
VERSION="$(VER_MAJOR).$(VER_MINOR)"
RELEASE=1

include Makefiles/install.mk

make_version:
	echo "/* Automatically generated */" > src/version.h
	echo "" >> src/version.h
	echo "#ifndef RFS_VERSION_H" >> src/version.h
	echo "#define RFS_VERSION_H" >> src/version.h
	echo "" >> src/version.h
	echo "#define RFS_VERSION_MAJOR $(VER_MAJOR)" >> src/version.h
	echo "#define RFS_VERSION_MINOR $(VER_MINOR)" >> src/version.h
	echo "#define RFS_VERSION $(VERSION)f" >> src/version.h
	echo "#define RFS_RELEASE $(RELEASE)" >> src/version.h
	echo "#define RFS_FULL_VERSION \"$(VERSION)-$(RELEASE)\"" >> src/version.h
	echo "#define RFS_BUILD_DATETIME \"$(BUILDDATETIME)\"" >> src/version.h
	echo "" >> src/version.h
	echo -n "static inline void print_version(void) { printf(\"%s (built on %s)\\" >> src/version.h
	echo "n\", RFS_FULL_VERSION, RFS_BUILD_DATETIME); }" >> src/version.h
	echo "" >> src/version.h
	echo "#endif /* RFS_VERSION_H */" >> src/version.h
	echo "" >> src/version.h

