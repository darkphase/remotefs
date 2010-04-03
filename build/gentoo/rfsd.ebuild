inherit eutils

DESCRIPTION="remote filesystem client"
HOMEPAGE="http://remotefs.sourceforge.net"
LICENSE="GPL"

IUSE="+ugo +exports_list ipv6 acl"
DEPEND="acl? ( sys-apps/acl )
	>=sys-fs/fuse-2.6
	virtual/libc"
RDEPEND="${DEPEND}"
SLOT="0"

KEYWORDS="~x86 ~mips ~mipsel ~ppc ~arm ~armeb ~ai64 ~amd64 ~x86_64"
SRC_URI="http://downloads.sourceforge.net/remotefs/remotefs-VERSION HERE.tar.bz2"

BUILDDIR=INSERT BUILDDIR HERE

setup_compile() {
    echo "" > "${BUILDDIR}/build/Makefiles/options.mk"
    if use ugo; then
	echo "OPT_1=-DWITH_UGO" >> "${BUILDDIR}/build/Makefiles/options.mk"
    fi
    if use exports_list; then
	echo "OPT_2=-DWITH_EXPORTS_LIST" >> "${BUILDDIR}/build/Makefiles/options.mk"
    fi
    if use ipv6; then
	echo "OPT_3=-DWITH_IPV6" >> "${BUILDDIR}/build/Makefiles/options.mk"
    fi
    if use acl; then
	echo "OPT_4=-DWITH_ACL" >> "${BUILDDIR}/build/Makefiles/options.mk"
	echo "OPT_4_LD=\$(LDFLAGS_ACL)" >> "${BUILDDIR}/build/Makefiles/options.mk"
    fi
    echo "CFLAGS_OPTS = \$(CFLAGS) \$(OPT_1) \$(OPT_2) \$(OPT_3) \$(OPT_4) \$(OPT_5)" >> "${BUILDDIR}/build/Makefiles/options.mk"
    echo "LDFLAGS_OPTS = \$(LDFLAGS) \$(OPT_2_LD) \$(OPT_3_LD) \$(OPT_4_LD) \$(OPT_5_LD)" >> "${BUILDDIR}/build/Makefiles/options.mk"
}

compile() {
    ALT="Gentoo" make -C "${BUILDDIR}/" rfsd rfspasswd rfsd_man
}

src_compile() {
    setup_compile
    compile
}

setup_install() {
    # install root
    echo "INSTALL_DIR=${D}/usr/" > "${BUILDDIR}/build/Makefiles/install.mk"
    
    mkdir -p "${D}/etc/init.d"
    mkdir -p "${D}/etc/conf.d"
}

make_install() {
    make -C "${BUILDDIR}/" install
    
    cp "${BUILDDIR}/build/etc/rfs-exports" "${D}/etc/"
    chmod 600 "${D}/etc/rfs-exports"
    chown root:root "${D}/etc/rfs-exports"
    
    cp "${BUILDDIR}/build/init.d/rfsd.gentoo" "${D}/etc/init.d/rfsd"
    chmod 700 "${D}/etc/init.d/rfsd"
    chown root:root "${D}/etc/init.d/rfsd"
    
    cp "${BUILDDIR}/build/gentoo/conf.d/rfsd" "${D}/etc/conf.d/rfsd"
    chmod 644 "${D}/etc/conf.d/rfsd"
    chown root:root "${D}/etc/conf.d/rfsd"
}

src_install() {
    setup_install
    make_install
}
