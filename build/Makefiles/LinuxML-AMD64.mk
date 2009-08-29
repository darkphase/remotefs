include build/Makefiles/Linux.mk

################################
# OS / CC specifics flags
################################

CFLAGS_OS      += -m64 -I/usr/include

###############################
# Flags for linking
###############################

LDFLAGS_DEBUG   += -m64 -L/usr/lib/
LDFLAGS_RELEASE += -m64 -L/usr/lib/

