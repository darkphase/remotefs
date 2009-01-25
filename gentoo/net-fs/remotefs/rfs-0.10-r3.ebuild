DESCRIPTION="remote filesystem client"
HOMEPAGE="http://remotefs.sourceforge.net"
LICENSE="GPL"
IUSE="ipv6 ssl"
DEPEND="ssl? ( >=dev-libs/openssl-0.9.8h ) 
	>=sys-fs/fuse-2.5"
SLOT="0"

KEYWORDS="x86 mips mipsel ppc arm ai64 amd64 x86_64"
SRC_URI="http://downloads.sourceforge.net/remotefs/remotefs-0.10-3.tar.bz2"

src_compile() {
    make -C remotefs-0.10-3/ rfs
}

src_install() {
    make -C remotefs-0.10-3/ install
}
