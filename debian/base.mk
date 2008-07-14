
all: builddeb

builddeb:
	@rm -fr "$(NAME)-$(VERSION)"
	@mkdir "$(NAME)-$(VERSION)"
	@rm -fr dpkg
	@mkdir -p "dpkg$(INSTALL_DIR)"
	@mkdir -p "dpkg/DEBIAN"
	@sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" $(CONTROL_TEMPLATE) >dpkg/DEBIAN/control.1
	@sed -e "s/AND SIZE HERE/$(SIZE)/" dpkg/DEBIAN/control.1 >dpkg/DEBIAN/control
	@rm -f dpkg/DEBIAN/control.1
	@mv "$(TARGET)" "dpkg$(INSTALL_DIR)"
	@echo "Building package $(NAME)_$(VERSION)_$(ARCH).deb"
	@dpkg -b dpkg "$(NAME)_$(VERSION)_$(ARCH).deb" >/dev/null
	@rm -fr dpkg
	@rm -fr "$(NAME)-$(VERSION)"
	