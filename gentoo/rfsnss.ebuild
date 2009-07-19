inherit eutils

DESCRIPTION="remote filesystem client"
HOMEPAGE="http://remotefs.sourceforge.net"
LICENSE="GPL"

IUSE=""
DEPEND="=net-fs/rfs-GENTOO VERSION
	virtual/libc"
SLOT="0"

KEYWORDS="~x86 ~mips ~mipsel ~ppc ~arm ~armeb ~ai64 ~amd64 ~x86_64"
SRC_URI="http://downloads.sourceforge.net/remotefs/remotefs-VERSION HERE.tar.bz2"

BUILDDIR=INSERT BUILDDIR HERE

setup_install() {
    # install root
    echo "INSTALL_DIR=${D}/usr/" > "${BUILDDIR}/Makefiles/install.mk"
}

src_compile() {
    ALT="Gentoo" make -C "${BUILDDIR}/" libnss nss nss_man
}

src_install() {
    setup_install

    make -C "${BUILDDIR}/" install_nss
}
