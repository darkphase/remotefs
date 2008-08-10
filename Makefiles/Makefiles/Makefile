
default: all

all: rfs rfsd rfspasswd

rfs:
	CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(MAKE) -f Makefiles/rfs.mk
	
rfsd:
	CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(MAKE) -f Makefiles/rfsd.mk
	
rfspasswd:
	CC="$(CC)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" $(MAKE) -f Makefiles/rfspasswd.mk
	
clean:
	$(MAKE) -f Makefiles/rfs.mk clean
	$(MAKE) -f Makefiles/rfsd.mk clean
	$(MAKE) -f Makefiles/rfspasswd.mk clean
	