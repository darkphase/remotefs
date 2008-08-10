##############################
# Get platform dependent flags
##############################
# gmake
OS=$(shell uname)
# Solaris, FreeBSD
OS:sh=uname
include Makefiles/$(OS)$(ALT).mk

#############################
# compile rules
#############################

all: flags $(TARGET)

%.o : %.c
	@echo Compiling $<
	@$(CC) -c $(CFLAGS) $< -o $@
	
$(TARGET): flags $(OBJS)
	@echo Linking $(TARGET)
	@$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

flags:
	@echo Flags for $(TARGET):
	@echo CC = $(CC)
	@echo CFLAGS = $(CFLAGS)
	@echo LDFLAGS = $(LDFLAGS)
	@echo

#############################
# Dependencies for all proj.
#############################

include Makefiles/depends.mk
