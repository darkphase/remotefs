CC=gcc
TARGET=../../nss_test

default:
	$(CC) -c nss_test.c -o nss_test.o
	$(CC) nss_test.o -o $(TARGET) -L../.. -lrfs

clean:
	rm -f $(TARGET)
	rm -f *.o

