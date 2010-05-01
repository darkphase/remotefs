
#############################
# Build man pages
#############################

rfs_man:
	mkdir -p build/man/gz/man1
	mkdir -p build/man/gz/man8
	gzip -c < build/man/rfs.1 > build/man/gz/man1/rfs.1.gz
	gzip -c < build/man/mount.rfs.8 > build/man/gz/man8/mount.rfs.8.gz

rfsd_man:
	mkdir -p build/man/gz/man8
	gzip -c < build/man/rfsd.8 > build/man/gz/man8/rfsd.8.gz
	gzip -c < build/man/rfspasswd.8 > build/man/gz/man8/rfspasswd.8.gz

rfsnss_man: dummy
	mkdir -p build/man/gz/man1
	gzip -c < build/man/rfs_nssd.1 > build/man/gz/man1/rfs_nssd.1.gz
	gzip -c < build/man/rfsnsswitch.sh.1 > build/man/gz/man1/rfsnsswitch.sh.1.gz

man: dummy rfs_man rfsd_man rfsnss_man

dummy:
