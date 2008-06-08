VERSION="0.8-3"
ARCH="i386"
NAME="rfsd"
CLIENT_FILES="../src/rfsd.c ../src/rfsd.h \
../src/rfspasswd.c \
../src/buffer.c ../src/buffer.h \
../src/command.c ../src/command.h \
../src/sendrecv.c ../src/sendrecv.h \
../src/server_handlers.c ../src/server_handlers.h \
../src/server_handlers_sync.c \
../src/signals.c ../src/signals.h \
../src/signals_server.c ../src/signals_server.h \
../src/alloc.c ../src/alloc.h \
../src/list.c ../src/list.h \
../src/passwd.c ../src/passwd.h \
../src/exports.c ../src/exports.h \
../src/crypt.c ../src/crypt.h \
../src/keep_alive_server.c ../src/keep_alive_server.h \
../src/config.h \
../src/inet.h"
MAKEFILE="Makefile.rfsd"
TARGET="rfsd"
PASSWD_TARGET="rfspasswd"
INSTALL_DIR="/usr/bin"
ETC_DIR="/etc"
ETC_FILES="../etc/rfs-exports"
INIT_SCRIPT="../init.d/rfsd.debian"
CONTROL_TEMPLATE="control.rfsd"

mkdir ${NAME}-${VERSION}

cp ${CLIENT_FILES} ${NAME}-${VERSION}/
cp ${MAKEFILE} ${NAME}-${VERSION}/Makefile

TARGET=../${TARGET} PASSWD_TARGET=../${PASSWD_TARGET} make -C ${NAME}-${VERSION}
SIZE_RFSD=`ls -s ${TARGET} | sed -e "s/\([0-9]*\).*/\1/"`
SIZE_RFSPASSWD=`ls -s ${PASSWD_TARGET} | sed -e "s/\([0-9]*\).*/\1/"`
SIZE=`echo ${SIZE_RFSD} + ${SIZE_RFSPASSWD} | bc`

rm -fr ${NAME}-${VERSION}

rm -fr dpkg
mkdir -p dpkg${INSTALL_DIR}
mkdir -p dpkg${ETC_DIR}
mkdir -p dpkg${ETC_DIR}/init.d
mkdir -p dpkg/DEBIAN
sed -e "s/INSERT ARCH HERE, PLEASE/${ARCH}/" ${CONTROL_TEMPLATE} >dpkg/DEBIAN/control.1
sed -e "s/AND SIZE HERE/${SIZE}/" dpkg/DEBIAN/control.1 >dpkg/DEBIAN/control
rm -f dpkg/DEBIAN/control.1
mv ${TARGET} dpkg${INSTALL_DIR}
mv ${PASSWD_TARGET} dpkg${INSTALL_DIR}
cp ${ETC_FILES} dpkg${ETC_DIR}
cp ${INIT_SCRIPT} dpkg${ETC_DIR}/init.d/${NAME}

dpkg -b dpkg ${NAME}_${VERSION}_${ARCH}.deb

rm -fr dpkg
