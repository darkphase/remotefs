default: htmlman

MANFILES =               \
	rfs.1            \
	rfsd.8           \
	rfspasswd.8      \
	rfs_nssd.1       \
	rfsnsswitch.sh.1 \
	mount.rfs.8      \

htmlman:
	@for FILE in $(MANFILES); \
	do \
		echo "Creating html/$$FILE.html"; \
		mkdir -p html/; \
		groff -m man -T html $$FILE > html/$$FILE.html; \
	done

clean:
	@for FILE in $(MANFILES); \
	do \
		rm -f "html/$$FILE.html"; \
	done
	@if [ -d html/ ]; \
	then \
		rmdir html/; \
	fi

