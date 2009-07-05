INCLUDES=-I../../rfs_nss/src -I../../src/
OBJS= \
	../../rfs_nss/src/client.c \
	../../rfs_nss/src/client_common.c \
	../../rfs_nss/src/common.c

default:
		gcc rfsnss_test.c $(OBJS) $(INCLUDES) -o rfsnss_test

clean:
		rm -f rfsnss_test

