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


if [ -z `which openssl` ]; then
	echo "You need openssl installed to enable SSL mode within remotefs"
fi

_UID=`id | sed 's/uid=\([0-9]*\)(.*/\1/'`
if [ $_UID -eq 0 ]; then
	KEY_FILE=/etc/rfsd-key.pem
	CERT_FILE=/etc/rfsd-cert.pem
else
	mkdir -p "$HOME/.rfs/"
	KEY_FILE="$HOME/.rfs/rfs-key.pem"
	CERT_FILE="$HOME/.rfs/rfs-cert.pem"
fi

echo "Creating key file at $KEY_FILE"
rm -f "$KEY_FILE"
openssl genrsa -out "$KEY_FILE" 1024
chmod 400 "$KEY_FILE"

echo "Creating certificate file at $CERT_FILE"
rm -f $CERT_FILE
openssl req -new -x509 -key "$KEY_FILE" -out "$CERT_FILE" -batch 2>/dev/null

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
