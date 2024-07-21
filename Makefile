include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=SKWTCPServer
PKG_RELEASE:=1
PKG_RELEASE:=1.0

PKG_BUILD_DIR := $(KERNEL_BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/SKWTCPServer
	SECTION:=utils
	CATEGORY:=SKWTCPServer
	TITLE:=TCPServer
	MAINTAINER:=JZY
endef

#TARGET_CFLAGS += -Wall

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

#define Build/Compile
	#$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
	#-o $(PKG_BUILD_DIR)/SKWTCPServer \
	#$(PKG_BUILD_DIR)/src/server.c
#endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
	$(TARGET_CONFIGURE_OPTS) \
	CFLAGS="$(TARGET_CFLAGS)" \
	CPPFLAGS="$(TARGET_CPPFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/SKWTCPServer/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/SKWTCPServer $(1)/bin/
endef

$(eval $(call BuildPackage,SKWTCPServer))
