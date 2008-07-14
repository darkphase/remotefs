
CFLAGS += -Wall -Werror

all: build
build: $(OBJS) link

%.o : %.c
	@echo "Compiling $<"
	@$(CC) -c $(CFLAGS) $< -o $@
	
link:
	@echo "Linking $(TARGET)"
	@$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJS)
	@rm -f $(TARGET)