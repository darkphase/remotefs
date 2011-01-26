
include build/Makefiles/install.mk

#############################
# man
#############################
install_man: dummy
	mkdir -p $(INSTALL_DIR)/share/man;
	if [ -d "build/man/gz" ]; then \
	    FILES="build/man/gz/*"; \
	    for GZ_FILE in "$$FILES"; \
	    do \
		cp -r $$GZ_FILE $(INSTALL_DIR)/share/man/; \
	    done \
	fi

uninstall_man: dummy
	if [ -d "build/man/gz" ]; then \
	    FILES="build/man/gz/*"; \
	    for GZ_FILE in "$$FILES"; \
	    do \
		    rm -f $$INSTALL_DIR/share/man/$$GZ_FILE; \
	    done \
	fi

#############################
# Installing
#############################
install_client:
	@$(MAKE) $(SILENT) -f build/Makefiles/librfs.mk install_librfs
	@$(MAKE) $(SILENT) -f build/Makefiles/rfs.mk install_rfs
install_server:
	@$(MAKE) $(SILENT) -f build/Makefiles/rfsd.mk install_rfsd
	@$(MAKE) $(SILENT) -f build/Makefiles/rfspasswd.mk install_rfspasswd
install: dummy install_client install_server
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk install_man

install_nss: dummy 
	@$(MAKE) $(SILENT) -f build/Makefiles/libnss.mk install_libnss
	@$(MAKE) $(SILENT) -f build/Makefiles/nssd.mk install_nssd
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk install_man

#############################
# Uninstalling
#############################
uninstall_client:
	@$(MAKE) $(SILENT) -f build/Makefiles/librfs.mk uninstall_librfs
	@$(MAKE) $(SILENT) -f build/Makefiles/rfs.mk uninstall_rfs
uninstall_server:
	@$(MAKE) $(SILENT) -f build/Makefiles/rfsd.mk uninstall_rfsd
	@$(MAKE) $(SILENT) -f build/Makefiles/rfspasswd.mk uninstall_rfspasswd
uninstall: dummy uninstall_client uninstall_server
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk uninstall_man

uninstall_nss: dummy
	@$(MAKE) $(SILENT) -f build/Makefiles/nssd.mk uninstall_nssd
	@$(MAKE) $(SILENT) -f build/Makefiles/libnss.mk uninstall_libnss
	@$(MAKE) $(SILENT) -f build/Makefiles/base.mk uninstall_man
