##############################
# Get platform dependent flags
##############################
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname
include Makefiles/$(OS)$(ALT).mk

#############################
# compile rules
#############################

all: flags $(TARGET)

.c.o:
	@echo Compiling $<
	@$(CC) -c $(CFLAGS) $< -o $@
	
$(TARGET): flags $(OBJS)
	@echo Linking $(TARGET)
	@$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

flags:
	@echo Flags for $(TARGET):
	@echo CC = $(CC)
	@echo CFLAGS = $(CFLAGS)
	@echo LDFLAGS = $(LDFLAGS)
	@echo

bldrpm:
	@( \
	if [ "$(MACHINE)" = "" ]; then MAC=`uname -m`; else NAC=$(MACHINE); fi ; \
	RPMD=`rpm -q --eval=%{_topdir} --specfile $(RPMNAME).spec | grep -v $(RPMNAME)`; \
	if [ -d "$$RPMD" -a -w "$$RPMD" ];\
	then \
	   VER=`sed -n 's/Version:\(.*\)/\1/p' rfsd.spec|tr -d ' '`; \
	   SDIR=`pwd`; \
	   NM=$(RPMNAME)-$$VER; \
	   mkdir /tmp/$$NM ;\
	   cp -r * /tmp/$$NM; \
	   find /tmp/$$NM -name .svn -exec rm -fr {} \; ; \
	   cd /tmp ; \
	   tar czf $$RPMD/SOURCES/$$NM.tar.gz $$NM ; \
	   cd $$SDIR ; \
	   if [ -x /usr/bin/rpmbuild ]; \
	   then \
	      rpmbuild -ba --target $$MAC /tmp/$$NM/$(RPMNAME).spec; \
	   else \
	      rpm -ba $$NM/$(RPMNAME).spec; \
	   fi; \
	   rm -fr /tmp/$$NM 2>/dev/tty 1>/dev/tty; \
	else \
	   echo You must be root for this.; \
	   echo Vous devez etre root pour ceci.; \
	   echo Sie muessen root sein.; \
	   echo Devi essere root per fare questo.; \
	   exit 2; \
	fi;\
	)

#############################
# Dependencies for all proj.
#############################

include Makefiles/depends.mk
