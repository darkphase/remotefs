
VERSION="0.10-2"
ARCH="i386"
NAME="rfs"

INSTALL_DIR="/usr/bin"
CONTROL_TEMPLATE="debian/control.rfs"

include debian/base.mk

TARGET="rfs"

SIZE = `ls -s ${TARGET} | sed -e "s/\([0-9]*\).*/\1/"`

default: all

