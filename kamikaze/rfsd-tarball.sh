VERSION="0.10"
SERVER_FILES="../src/rfsd.h ../src/rfsd.c \
	../src/rfspasswd.c \
	../src/buffer.h ../src/buffer.c \
	../src/command.h ../src/command.c \
	../src/sendrecv.h ../src/sendrecv.c \
	../src/server_handlers.h ../src/server_handlers.c \
	../src/server_handlers_sync.c \
	../src/signals.h ../src/signals.c \
	../src/signals_server.h ../src/signals_server.c \
	../src/alloc.h ../src/alloc.c \
	../src/list.h ../src/list.c \
	../src/passwd.h ../src/passwd.c \
	../src/exports.h ../src/exports.c \
	../src/crypt.h ../src/crypt.c \
	../src/keep_alive_server.h ../src/keep_alive_server.c \
	../src/config.h \
	../src/inet.h \
	../src/path.h ../src/path.c \
	../LICENSE ../AUTHORS ../CHANGELOG"
MAKEFILE="Makefile.rfsd"
NAME="rfsd"
INIT_SCRIPT="../init.d/rfsd.kamikaze"
ETC_FILES="../etc/rfs-exports"

rm -fr ${NAME}_${VERSION} ${NAME}_${VERSION}.tar.bz2
rm -f ${NAME}_${VERSION}.md5
mkdir ${NAME}-${VERSION}
mkdir ${NAME}-${VERSION}/init.d/
mkdir ${NAME}-${VERSION}/etc/

cp ${INIT_SCRIPT} ${NAME}-${VERSION}/init.d/${NAME}
cp ${ETC_FILES} ${NAME}-${VERSION}/etc/
chmod 600 ${NAME}-${VERSION}/etc/*

cp ${SERVER_FILES} ${NAME}-${VERSION}/
cp ${MAKEFILE} ${NAME}-${VERSION}/Makefile
tar -cpjf ${NAME}_${VERSION}.tar.bz2 ${NAME}-${VERSION}
md5sum ${NAME}_${VERSION}.tar.bz2 >${NAME}_${VERSION}.md5

rm -fr ${NAME}-${VERSION}
