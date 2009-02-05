inherit eutils

DESCRIPTION="remote filesystem client"
HOMEPAGE="http://remotefs.sourceforge.net"
LICENSE="GPL"

IUSE="ipv6 ssl"
DEPEND="ssl? ( >=dev-libs/openssl-0.9.8h )"
SLOT="0"

KEYWORDS="~x86 ~mips ~mipsel ~ppc ~arm ~armeb ~ai64 ~amd64 ~x86_64"
SRC_URI="http://downloads.sourceforge.net/remotefs/remotefs-0.11-1.tar.bz2"

BUILDDIR="remotefs-0.11-1"

setup_compile() {
    if use ipv6; then
	echo "OPT_1=-DWITH_IPV6" > "${BUILDDIR}/Makefiles/options.mk"
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
    
    mkdir -p ${D}/etc/
}

src_compile() {
    setup_compile

    ALT="Gentoo" make -C "${BUILDDIR}/" rfsd rfspasswd rfsd_man
}

src_install() {
    setup_install

    make -C "${BUILDDIR}/" install
    cp "${BUILDDIR}/etc/rfs-exports" ${D}/etc/
}
