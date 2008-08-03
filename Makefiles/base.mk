
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname

include Makefiles/$(OS).mk

CFLAGS += $(OS_SPECIFIC_CFLAGS)
CFLAGS += -Wall -Werror

all: build
build: $(OBJS) link

%.o : %.c
	@echo "Compiling $< [$(CFLAGS)]"
	@$(CC) -c $(CFLAGS) $< -o $@
	
link:
	@echo "Linking $(TARGET) [$(LDFLAGS)]"
	@$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJS)
	@rm -f $(TARGET)