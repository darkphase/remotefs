include Makefiles/Linux.mk

ARCH=i386

################################
# OS / CC specifics flags
################################

CFLAGS_OS      += -m32 -I/usr/i486-linux-gnu/include

###############################
# Flags for linking
###############################

LDFLAGS_DEBUG   += -m32 -L/usr/i486-linux-gnu/lib/
LDFLAGS_RELEASE += -m32 -L/usr/i486-linux-gnu/lib/

