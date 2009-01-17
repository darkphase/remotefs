VERSION="0.11"
SERVER_FILES="../src/rfsd.h ../src/rfsd.c \
	../src/rfspasswd.c \
	../src/buffer.h ../src/buffer.c \
	../src/command.h ../src/command.c \
	../src/sendrecv.h ../src/sendrecv.c \
	../src/server_handlers.h ../src/server_handlers.c \
	../src/server_handlers_read.c \
	../src/server_handlers_sync.c \
	../src/signals.h ../src/signals.c \
	../src/signals_server.h ../src/signals_server.c \
	../src/list.h ../src/list.c \
	../src/passwd.h ../src/passwd.c \
	../src/exports.h ../src/exports.c \
	../src/crypt.h ../src/crypt.c \
	../src/keep_alive_server.h ../src/keep_alive_server.c \
	../src/compat.h \
	../src/config.h \
	../src/instance.h ../src/instance.c \
	../src/rfs_semaphore.h \
	../src/inet.h \
	../src/ratio.h \
	../src/path.h ../src/path.c \
	../src/id_lookup.h ../src/id_lookup.c \
	../src/sockets.h ../src/sockets.c \
	../src/server.h ../src/server.c \
	../src/cleanup.h ../src/cleanup.c \
	../src/rfs_errno.h ../src/rfs_errno.c \
	../src/utils.h ../src/utils.c \
	../src/sug_server.h ../src/sug_server.c \
	../LICENSE ../AUTHORS ../CHANGELOG"
MD5CRYPT_FILES="../src/md5crypt/crypt_md5.h ../src/md5crypt/crypt_md5.c \
	../src/md5crypt/md5.h ../src/md5crypt/md5.c \
	../src/md5crypt/README \
	../src/md5crypt/ORIGIN \
	../src/md5crypt/LSM"
MAKEFILE="Makefile.rfsd"
NAME="rfsd"
INIT_SCRIPT="../init.d/rfsd.kamikaze"
ETC_FILES="../etc/rfs-exports"

rm -fr ${NAME}_${VERSION} ${NAME}_${VERSION}.tar.bz2
rm -f ${NAME}_${VERSION}.md5
mkdir ${NAME}-${VERSION}
mkdir ${NAME}-${VERSION}/md5crypt/
mkdir ${NAME}-${VERSION}/init.d/
mkdir ${NAME}-${VERSION}/etc/

cp ${INIT_SCRIPT} ${NAME}-${VERSION}/init.d/${NAME}
cp ${ETC_FILES} ${NAME}-${VERSION}/etc/
chmod 600 ${NAME}-${VERSION}/etc/*

cp ${SERVER_FILES} ${NAME}-${VERSION}/
cp ${MD5CRYPT_FILES} ${NAME}-${VERSION}/md5crypt
cp ${MAKEFILE} ${NAME}-${VERSION}/Makefile
tar -cpjf ${NAME}_${VERSION}.tar.bz2 ${NAME}-${VERSION}
md5sum ${NAME}_${VERSION}.tar.bz2 >${NAME}_${VERSION}.md5

rm -fr ${NAME}-${VERSION}
