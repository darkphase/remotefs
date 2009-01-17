# 

include $(TOPDIR)/rules.mk

PKG_NAME:=rfsd
PKG_VERSION:=0.11
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://z-z-z.org.ua/rfs
PKG_MD5SUM:=e45f44fb5b1f527730cb1297425ce798

include $(INCLUDE_DIR)/package.mk

define Package/rfsd
  DEPENDS:=
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
		CFLAGS="-fhonour-copts -O2 -D_FILE_OFFSET_BITS=64"\
		RFSD_LDFLAGS="-s" \
		RFSPASSWD_LDFLAGS="-s" \
		LIBRFSD_LDFLAGS="-s" \
		all
	rm -rf $(PKG_INSTALL_DIR)
	mkdir -p $(PKG_INSTALL_DIR)
	$(MAKE) -C $(PKG_BUILD_DIR) \
		INSTALL_DIR="$(PKG_INSTALL_DIR)/usr" \
		install
endef

define Package/rfsd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/rfsd $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/rfspasswd $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/librfsd.so $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) $(PKG_BUILD_DIR)/init.d/* $(1)/etc/init.d/
	$(CP) $(PKG_BUILD_DIR)/etc/* $(1)/etc/
endef

$(eval $(call BuildPackage,rfsd))
