#!/bin/sh

# read some variable
. Makefiles/version.mk

# may be OS dependent, first preset some variables

BIN=$INSTALL_DIR/bin
LIB=$INSTALL_DIR/lib
MAN=$INSTALL_DIR/share/man
ETC=/etc
INITD=/etc/rc.d/init.d
LIBEXT=so
RM="/bin/rm -f"
PKILL=/usr/bin/pkill

# and modify the OS dependent stufs
case `uname` in
Darwin)
    INITD=
    LIBEXT=dylib
    PKILL="killall -TERM"
    ;;
QNX)
    pkill()
    {
       PID=`ps -e | grep $1 | grep -v grep | awk '{ print $1 }'`
       if [ "x$PID" != x ]
       then
           kill $PID
       fi
    }
    PKILL=pkill
    ;;
esac

DAEMON=rfsd

RFSD_FILES="$BIN/rfsd
            $BIN/rfsdpasswd
            $ETC/rfs-exports
            $INITD/rfsd
"

RFS_FILES="$BIN/rfs
           $LIB/librfs.$LIBEXT
           $LIB/librfs.$LIBEXT.$VERSION
           $LIB/librfs.$VERSION.$LIBEXT
           $MAN/man1/man1/rfs.1.gz
           $MAN/man1/man7/remotefs.7.gz
           $MAN/man1/man8/rfsd.8.gz
           $MAN/man1/man8/rfspasswd.8.gz
"

$PKILL $DAEMON
IFS=' '
echo "$RFSD_FILES $RFS_FILES" | while read file
do
    if [ "x$file" != x -a -f "$file" ]
    then
        $RM $file
    fi
done

