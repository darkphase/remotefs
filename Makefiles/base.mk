##############################
# Get platform dependent flags
##############################
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname
include Makefiles/$(OS)$(ALT).mk

#############################
# Objects and targets
#############################

include Makefiles/rfs.mk
include Makefiles/rfsd.mk
include Makefiles/rfspasswd.mk

#############################
# Version for packages
#############################
include Makefiles/version.mk

#############################
# The begin of the world
#############################

#############################
# compile rules for objects
#############################

$(com1_OBJS) $(com2_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(rfsd_CFLAGS) $(DRF);

$(rfs_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(rfs_CFLAGS) $(DRF);

$(rfsd_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(rfsd_CFLAGS) $(DRF);

$(rfspasswd_OBJS):
	@echo Compile $@
	$(CC) -c -o $@ $*.c $(rfspasswd_CFLAGS) $(DRF);

#######################################
# Rules for compiling programs
#######################################

rfs: rfs_flag $(rfs_OBJS) $(com1_OBJS) $(com2_OBJS)
	@if [ "`pkg-config --cflags fuse 2> /dev/null`" != "" ]; then \
	         echo Link $@; \
	         $(CC) -o $@ $(rfs_OBJS) $(com1_OBJS) $(com2_OBJS) $($(@)_LDFLAGS) $($(@)_CFLAGS); \
	else \
	        echo "!!! fuse not installed or not found, skip rfs !!!"; \
	fi; \
	echo

rfsd: rfsd_flag $(rfsd_OBJS) $(com1_OBJS) $(com2_OBJS)
	@echo Link $@
	@$(CC) -o $@ $(rfsd_OBJS) $(com1_OBJS) $(com2_OBJS) $($(@)_LDFLAGS) $($(@)_CFLAGS)
	@echo

rfspasswd: rfspasswd_flag  $(rfspasswd_OBJS) $(com1_OBJS)
	@echo Link $@;
	@$(CC) -o $@ $(rfspasswd_OBJS) $(com1_OBJS) $($(@)_LDFLAGS) $($(@)_CFLAGS)
	@echo

runtests: 
	@$(MAKE) -sC tests run
	@$(MAKE) -sC tests clean


#############################
# remove all temporaries objects
#############################

clean:
	@if ls src/*.o >/dev/null 2>&1; then $(RM) -f src/*.o; fi
	@if ls *.deb >/dev/null 2>&1; then $(RM) -f *.deb; fi
	@if [ -f rfs ]; then $(RM) -f rfs; fi
	@if [ -f rfsd ]; then $(RM) -f rfsd; fi
	@if [ -f rfspasswd ]; then $(RM) -f rfspasswd; fi

#############################
# Rebuild dependency file
#############################
depends:
	@grep 'include *".*"' src/*.c | sed -e 's/\.c/.o/' -e 's/#include *"\(.*\.[ch]\)"/src\/\1/' > Makefiles/depends.mk
	@ls src/*.c | sed -e 's/\([^\.]*\)/\1.o:\1/' >> Makefiles/depends.mk

#############################
# Print out the flags we use
#############################

rfs_flag:
	@echo Flags for rfs
	echo CFLAGS = $(rfs_CFLAGS) $(DRF)
	@echo LDFLAGS = $(rfs_LDFLAGS)
	@echo

rfsd_flag:
	@echo Flags for rfsd
	echo CFLAGS = $(rfsd_CFLAGS) $(DRF)
	@echo LDFLAGS = $(rfsd_LDFLAGS)
	@echo

rfspasswd_flag:
	@echo Flags for rfspasswd
	echo CFLAGS = $(rfspasswd_CFLAGS) $(DRF)
	@echo LDFLAGS = $(rfspasswd_LDFLAGS)
	@echo

#######################################
# Rules for packaging, ...
#######################################

#############################
# Build deb packages
#############################

rpm: rpm-rfsd rpm-rfs


debrelease: rfsdeb rfsddeb

rfsdeb: rfs
	@CONTROL_TEMPLATE="debian/control.rfs" TARGET="$?" \
	NAME=$< \
	make -f Makefiles/base.mk builddeb

rfsddeb: rfsd rfspasswd
	@CONTROL_TEMPLATE="debian/control.rfsd" TARGET="$?" \
	NAME=$< \
	$(MAKE) -sf Makefiles/base.mk builddeb

builddeb:
	@rm -fr "$(NAME)-$(VERSION)-$(RELEASE)";\
	mkdir "$(NAME)-$(VERSION)-$(RELEASE)";\
	rm -fr dpkg;\
	mkdir -p "dpkg$(INSTALL_DIR)";\
	mkdir -p "dpkg/DEBIAN";\
	sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" $(CONTROL_TEMPLATE) >dpkg/DEBIAN/control.1;\
	SIZE=`cat $(TARGET)|wc -c`; sed -e "s/AND SIZE HERE/$$SIZE/" dpkg/DEBIAN/control.1 >dpkg/DEBIAN/control;\
	rm -f dpkg/DEBIAN/control.1;\
	mv $(TARGET) "dpkg$(INSTALL_DIR)";\
	echo "Building package $(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb";\
	dpkg -b dpkg "$(NAME)_$(VERSION)-$(RELEASE)_$(ARCH).deb" >/dev/null;\
	rm -fr dpkg;\
	rm -fr "$(NAME)-$(VERSION)-$(RELEASE)"



#############################
# Build RPM
#############################

rpm: rpm-rfsd rpm-rfs

rpm-rfsd: clean
	@RPMNAME=rfsd $(MAKE) -sf Makefiles/base.mk bldrpm
	
rpm-rfs:  clean
	@RPMNAME=rfs $(MAKE) -sf Makefiles/base.mk bldrpm

rpms/%.spec: Makefiles/version.mk
	@echo rebuild spec file
	@sed -e "s/Version:.*/Version:$(VERSION)/"  \
	    -e "s/Release:.*/Release:$(RELEASE)/" $@ \
	    > rpms/tmp; mv rpms/tmp $@


bldrpm: rpms/$(RPMNAME).spec
	@if [ "$(MACHINE)" = "" ] ; then MAC=$(ARCH); else MAC=$(MACHINE); fi ; \
	RPMD=`rpm -q --eval=%{_topdir} --specfile rpms/$(RPMNAME).spec | grep -v $(RPMNAME)`; \
	if [ -d "$$RPMD" -a -w "$$RPMD" ];\
	then \
	   VER=`sed -n 's/Version:\(.*\)/\1/p' rpms/$(RPMNAME).spec|tr -d ' '`; \
	   SDIR=`pwd`; \
	   NM=$(RPMNAME)-$$VER; \
	   mkdir /tmp/$$NM ;\
	   cp -r * /tmp/$$NM; \
	   find /tmp/$$NM -name .svn -exec rm -fr {} \; 2>/dev/null; \
	   cd /tmp ; \
	   tar czf $$RPMD/SOURCES/$$NM.tar.gz $$NM ; \
	   cd $$SDIR ; \
	   if [ -x /usr/bin/rpmbuild ]; \
	   then \
	      rpmbuild -ba --target $$MAC /tmp/$$NM/rpms/$(RPMNAME).spec; \
	   else \
	      rpm -ba $$NM/rpms/$(RPMNAME).spec; \
	   fi; \
	   rm -fr /tmp/$$NM 2>/dev/tty 1>/dev/tty; \
	else \
	   echo You must be root for this.; \
	   exit 2; \
	fi;\
	

#############################
# Dependencies for all proj.
#############################

include Makefiles/depends.mk
