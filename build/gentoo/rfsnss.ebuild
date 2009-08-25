inherit eutils

DESCRIPTION="remote filesystem client"
HOMEPAGE="http://remotefs.sourceforge.net"
LICENSE="GPL"

IUSE=""
DEPEND="=net-fs/rfs-GENTOO VERSION
	virtual/libc
	sys-apps/sed"
RDEPEND="${DEPEND}"
SLOT="0"

KEYWORDS="~x86 ~mips ~mipsel ~ppc ~arm ~armeb ~ai64 ~amd64 ~x86_64"
SRC_URI="http://downloads.sourceforge.net/remotefs/remotefs-VERSION HERE.tar.bz2"

BUILDDIR=INSERT BUILDDIR HERE

setup_install() {
    # install root
    echo "INSTALL_DIR=${D}/usr/" > "${BUILDDIR}/Makefiles/install.mk"
    
    mkdir -p "${D}/usr/sbin/"
}

src_compile() {
    ALT="Gentoo" make -C "${BUILDDIR}/" libnss nss rfsnss_man
}

src_install() {
    setup_install

    make -C "${BUILDDIR}/" install_nss
    
    cp "${BUILDDIR}/build/sbin/rfsnsswitch.sh" "${D}/usr/sbin/"
    chmod 700 "${D}/usr/sbin/rfsnsswitch.sh"
    chown root:root "${D}/usr/sbin/rfsnsswitch.sh"
}

pkg_postinst() {
    rfsnsswitch.sh install
}

pkg_prerm() {
    rfsnsswitch.sh uninstall
}
