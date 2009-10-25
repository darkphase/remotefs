include build/Makefiles/packaging/tbz.mk
include build/Makefiles/packaging/debian.mk
include build/Makefiles/packaging/redhat.mk
include build/Makefiles/packaging/gentoo.mk
include build/Makefiles/packaging/kamikaze.mk
include build/Makefiles/packaging/installing.mk

clean_packages_tmp: dummy clean_tbz_tmp clean_debian_tmp clean_redhat_tmp clean_gentoo_tmp clean_kamikaze_tmp
clean_packages: dummy clean_tbz clean_debian clean_redhat clean_gentoo clean_kamikaze
