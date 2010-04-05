#!/bin/sh

NSSWITCH_FILE="/etc/nsswitch.conf"

backup_nsswitch()
{
	NSSWITCH_BACKUP="$NSSWITCH_FILE.bak"

	if ! cp "$NSSWITCH_FILE" "$NSSWITCH_BACKUP"
	then
		exit 1
	fi
	echo "$NSSWITCH_FILE backed up to $NSSWITCH_BACKUP"
}

insert_rfs()
{
	backup_nsswitch ;

	if ! grep -e '^passwd.* rfs' "$NSSWITCH_FILE" > /dev/null
	then
    	sed -e 's/^\(passwd:.*\)/\1 rfs/' < "$NSSWITCH_FILE" > "/tmp/`basename $NSSWITCH_FILE`"
		if ! mv -f "/tmp/`basename $NSSWITCH_FILE`" "$NSSWITCH_FILE"
		then 
			exit 1
		fi
	fi

	if ! grep -e '^group:.* rfs' "$NSSWITCH_FILE" > /dev/null
	then
    	sed -e 's/^\(group:.*\)/\1 rfs/' < "$NSSWITCH_FILE" > "/tmp/`basename $NSSWITCH_FILE`"
		if ! mv -f "/tmp/`basename $NSSWITCH_FILE`" "$NSSWITCH_FILE"
		then
			exit 1
		fi
	fi
}

remove_rfs()
{
	backup_nsswitch ;

	if grep rfs "$NSSWITCH_FILE" > /dev/null
	then
		sed -e 's/ rfs//' < "$NSSWITCH_FILE" > "/tmp/`basename $NSSWITCH_FILE`"
		if ! mv "/tmp/`basename $NSSWITCH_FILE`" "$NSSWITCH_FILE"
		then
			exit 1
		fi
	fi
}

case "$1" in
	install)   insert_rfs ;;
	uninstall) remove_rfs ;;
	*) echo `basename "$1"` 'install|uninstall' ;;
esac

exit 0
