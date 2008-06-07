# 

include $(TOPDIR)/rules.mk

PKG_NAME:=rfsd
PKG_VERSION:=0.8
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://z-z-z.org.ua/rfs
PKG_MD5SUM:=

include $(INCLUDE_DIR)/package.mk

define Package/rfsd
  DEPENDS:=+libgcrypt
  SECTION:=net
  CATEGORY:=Network
  TITLE:=remote filesystem daemon and passwd utility
  URL:=http://z-z-z.org.ua/rfs
endef

define Package/rfsd/description
 remote filesystem daemon and passwd utility
endef

define Build/Configure
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS) \
		CC="$(TARGET_CROSS)gcc" \
		CFLAGS="-fhonour-copts -O2 "\
		RFSD_LDFLAGS="-s" \
		RFSPASSWD_LDFLAGS="-lcrypt -s" \
		all
	rm -rf $(PKG_INSTALL_DIR)
	mkdir -p $(PKG_INSTALL_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR) \
		INSTALL_DIR="$(PKG_INSTALL_DIR)/usr/bin" \
		install
endef

define Package/rfsd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/rfsd $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/rfspasswd $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) $(PKG_BUILD_DIR)/init.d/* $(1)/etc/init.d/
	$(CP) $(PKG_BUILD_DIR)/etc/* $(1)/etc/
endef

$(eval $(call BuildPackage,rfsd))
