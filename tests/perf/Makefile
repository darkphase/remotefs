# GET Plattform dependent flags
OS=$(shell uname)
OS:sh=uname
include mk/$(OS).mk

OBJ=perf.o getopts.o

all: perf

perf: $(OBJ)
	$(CC) -o perf $(OBJ) $(CFLAGS) $(LDFLAGS)

remount: remount.o
	$(CC) -o romount $(OBJ) $(CFLAGS) $(LDFLAGS)
clean:
	rm perf remount $(OBJ)
