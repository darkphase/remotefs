inherit eutils

DESCRIPTION="remote filesystem client"
HOMEPAGE="http://remotefs.sourceforge.net"
LICENSE="GPL"

IUSE="ipv6 ssl"
DEPEND="ssl? ( >=dev-libs/openssl-0.9.8h ) 
	>=sys-fs/fuse-2.6"
SLOT="0"

KEYWORDS="~x86 ~mips ~mipsel ~ppc ~arm ~armeb ~ai64 ~amd64 ~x86_64"
SRC_URI="http://downloads.sourceforge.net/remotefs/remotefs-VERSION HERE.tar.bz2"

BUILDDIR=INSERT BUILDDIR HERE

setup_compile() {
    echo "" > "${BUILDDIR}/Makefiles/options.mk"
    if use ipv6; then
	echo "OPT_1=-DWITH_IPV6" >> "${BUILDDIR}/Makefiles/options.mk"
    fi
    if use ssl; then
	echo "OPT_2=-DWITH_SSL" >> "${BUILDDIR}/Makefiles/options.mk"
	echo "OPT_2_LD=\$(LDFLAGS_SSL)" >> "${BUILDDIR}/Makefiles/options.mk"
    fi
    echo "CFLAGS_OPTS = \$(CFLAGS) \$(OPT_1) \$(OPT_2)" >> "${BUILDDIR}/Makefiles/options.mk"
    echo "LDFLAGS_OPTS = \$(LDFLAGS) \$(OPT_2_LD)" >> "${BUILDDIR}/Makefiles/options.mk"
}

setup_install() {
    # install root
    echo "INSTALL_DIR=${D}/usr/" > "${BUILDDIR}/Makefiles/install.mk"
}

src_compile() {
    setup_compile

    ALT="Gentoo" make -C "${BUILDDIR}/" rfs rfs_man
}

src_install() {
    setup_install

    make -C "${BUILDDIR}/" install
}
