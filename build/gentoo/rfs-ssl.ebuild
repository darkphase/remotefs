inherit eutils

DESCRIPTION="remotefs certificates script for rfs and rfsd"
HOMEPAGE="http://remotefs.sourceforge.net"
LICENSE="GPL"

IUSE=""
DEPEND="dev-libs/openssl"
RDEPEND="${DEPEND}"
SLOT="0"

KEYWORDS="~x86 ~mips ~mipsel ~ppc ~arm ~armeb ~ai64 ~amd64 ~x86_64"
SRC_URI="http://downloads.sourceforge.net/remotefs/remotefs-VERSION HERE.tar.bz2"

BUILDDIR=INSERT BUILDDIR HERE

setup_install() {
    mkdir -p "${D}/usr/sbin/"
    mkdir -p "${D}/usr/share/man/man1/"
}

install_man() {
    cp "${BUILDDIR}/build/man/rfscert.sh.1" > "${D}/usr/share/man/man1/"
}

install() {
    cp "${BUILDDIR}/build/sbin/rfscert.sh" "${D}/usr/sbin/rfscert.sh"
    chmod 755 "${D}/usr/sbin/rfscert.sh"
    chown root:root "${D}/usr/sbin/rfscert.sh"
}

src_install() {
    setup_install
    install_man
    install
}
