
VERSION="0.10-2"
ARCH="i386"
NAME="rfsd"

INSTALL_DIR="/usr/bin"
CONTROL_TEMPLATE="debian/control.rfsd"

include debian/base.mk

PASSWD_TARGET="rfspasswd"
RFSD_TARGET="rfsd"

SIZE_RFSD = $(shell ls -s "$(RFSD_TARGET)" | sed -e "s/\([0-9]*\).*/\1/")
SIZE_RFSPASSWD = $(shell ls -s "$(PASSWD_TARGET)" | sed -e "s/\([0-9]*\).*/\1/")
SIZE = $(shell echo "$(SIZE_RFSD)" + "$(SIZE_RFSPASSWD)" | bc)

TARGET="$(RFSD_TARGET) $(PASSWD_TARGET)"

default: all
