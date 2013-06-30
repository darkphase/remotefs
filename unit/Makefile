
default: build

include build/Makefiles/variable/verbosity.mk
include build/Makefiles/variable/custom.mk

depends: dummy
	@$(MAKE) $(SILENT) -f base.mk depends

build: dummy
	@$(MAKE) $(SILENT) -f base.mk build

compile: dummy
	@$(MAKE) $(SILENT) -f base.mk compile

run: dummy
	@$(MAKE) $(SILENT) -f base.mk run

clean: dummy
	@$(MAKE) $(SILENT) -f base.mk clean

dummy:
