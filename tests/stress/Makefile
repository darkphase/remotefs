TARGET=tests
OBJS=file.o \
    test_read.o \
    main.o

default: clean $(OBJS)
	gcc $(OBJS) -o $(TARGET)

$(OBJS):
	gcc -c -o $@ $*.c -Wall -Werror

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)