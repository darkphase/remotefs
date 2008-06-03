VERSION="0.6-2"
ARCH=`uname -m`
NAME="rfs"
CLIENT_FILES="../src/rfs.c \
../src/operations.c ../src/operations.h \
../src/buffer.c ../src/buffer.h \
../src/command.c ../src/command.h \
../src/sendrecv.c ../src/sendrecv.h \
../src/signals.c ../src/signals.h \
../src/signals_client.c ../src/signals_client.h \
../src/alloc.c ../src/alloc.h \
../src/attr_cache.c ../src/attr_cache.h \
../src/list.c ../src/list.h \
../src/passwd.c ../src/passwd.h \
../src/crypt.c ../src/crypt.h \
../src/config.h \
../src/inet.h"
MAKEFILE="Makefile.rfs"
TARGET="rfs"
INSTALL_DIR="/usr/bin"
CONTROL_TEMPLATE="control.rfs"

mkdir ${NAME}-${VERSION}

cp ${CLIENT_FILES} ${NAME}-${VERSION}/
cp ${MAKEFILE} ${NAME}-${VERSION}/Makefile

TARGET=../${TARGET} make -C ${NAME}-${VERSION}
SIZE=`ls -s ${TARGET} | sed -e "s/\([0-9]*\).*/\1/"`

rm -fr ${NAME}-${VERSION}
rm -fr dpkg
mkdir -p dpkg${INSTALL_DIR}
mkdir -p dpkg/DEBIAN
sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" ${CONTROL_TEMPLATE} >dpkg/DEBIAN/control.1
sed -e "s/AND SIZE HERE/${SIZE}/" dpkg/DEBIAN/control.1 >dpkg/DEBIAN/control
rm -f dpkg/DEBIAN/control.1
mv ${TARGET} dpkg${INSTALL_DIR}

dpkg -b dpkg ${NAME}-${VERSION}-${ARCH}.deb

rm -fr dpkg
