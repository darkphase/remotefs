##############################
# Get platform dependent flags
##############################
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname
include Makefiles/$(OS)$(ALT).mk

#############################
# for debug messages set this
#############################
#DEBUG_CFLAGS=-DRFS_DEBUG -g

#############################
# The begin of the world
#############################

#CFLAGS += "$(DEBUG_CFLAGS)"


all: rfspasswd rfsd rfspasswd rfs 

release: rfsdeb rfsddeb

rfs: dummy
	@if [ "`pkg-config --cflags fuse 2> /dev/null`" != "" ]; then \
	        CFLAGS_G="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/rfs.mk rfs; echo ; \
	else \
	        echo "!!! fuse not installed or not found, skip rfs !!!"; echo ;\
	fi

rfsd: dummy
	@CFLAGS_G="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/rfsd.mk rfsd; echo
	
rfspasswd: dummy
	@CFLAGS_G="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/rfspasswd.mk rfspasswd; echo
	
rfsdeb: cleanup
	@$(MAKE) -sf debian/Makefile rfsdeb

rfsddeb: cleanup
	@$(MAKE) -sf debian/Makefile rfsddeb

runtests: 
	@$(MAKE) -sC tests run
	@$(MAKE) -sC tests clean


cleanup: clean
	@$(MAKE) -sf debian/Makefile clean
	
clean:
	@if ls src/*.o >/dev/null 2>&1; then $(RM) src/*.o; fi
	@if [ -f rfs ]; then $(RM) rfs; fi
	@if [ -f rfsd ]; then $(RM) rfsd; fi
	@if [ -f rfspasswd ]; then $(RM) rfspasswd; fi


dummy: # Force the call of the variuos Makefiles
	@:

flags:
	@CFLAGS_G="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/rfs.mk flags
	@CFLAGS_G="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/rfsd.mk flags
	@CFLAGS_G="$(DEBUG_CFLAGS)" $(MAKE) -sf Makefiles/rfspasswd.mk flags

depends:
	@grep 'include *".*"' src/*.c | sed -e 's/\.c/.o/' -e 's/#include *"\(.*\.h\)"/src\/\1/' > Makefiles/depends.mk
