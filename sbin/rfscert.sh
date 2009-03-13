#env sh

if [ -z `which openssl` ]; then
	echo "You need openssl installed to enable SSL mode within remotefs"
fi

create_client_cert() {
	mkdir -p "$HOME/.rfs/";
	KEY_FILE="$HOME/.rfs/rfs-key.pem";
	CERT_FILE="$HOME/.rfs/rfs-cert.pem";
	create_cert && check_cert;
}

create_server_cert() {
	KEY_FILE=/etc/rfsd-key.pem;
	CERT_FILE=/etc/rfsd-cert.pem;
	create_cert && check_cert;
}

create_cert() {
	echo "Creating key file at $KEY_FILE"
	rm -f "$KEY_FILE"
	openssl genrsa -out "$KEY_FILE" 1024
	chmod 400 "$KEY_FILE"

	echo "Creating certificate file at $CERT_FILE"
	rm -f $CERT_FILE
	openssl req -new -x509 -key "$KEY_FILE" -out "$CERT_FILE" -batch 2>/dev/null
}

make_conf()
{
cat <<!
dir=.
[req]
distinguished_name=rfs_distinguished_name
prompt=no
[rfs_distinguished_name]
C  = ..
ST = .
L  = .
O  = .
CN = .
!
}

check_cert() {
	if [ -f $CERT_FILE ]
	then
		: # this is correct !
	else
		# the cert file was not created, probably due to a wrong configuration
		# of the openssl.cnf file, create our own and try again.
		make_conf > openssl.cnf
		openssl req -new -x509 -key "$KEY_FILE" -out "$CERT_FILE" -batch -config openssl.cnf
		rm openssl.cnf
	fi
	chmod 400 "$CERT_FILE"
}

_UID=`id | sed 's/uid=\([0-9]*\)(.*/\1/'`
if [ $_UID -eq 0 ]; then
	echo "You're may want to:";
	echo "[1]\t\t - Create server's ceritficate";
	echo "[2]\t\t - Create client's certificate";
	echo "[anything else]\t - Quit";
	echo "";

	read "CHOICE"

	case "$CHOICE" in
		1)
		create_server_cert;
		;;
		2)
		create_client_cert;
		;;
		*)
		;;
	esac
else
	echo "You're about to create clien't certificate";
	echo "If you want to create server's certificate - you need to run this script with root privleges";
	create_client_cert;
fi

