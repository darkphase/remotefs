include build/Makefiles/SunOS.mk

################################
# The executables
################################

CC = gcc

################################
# OS / CC specifics flags
################################

CFLAGS_OS      = -DSOLARIS \
                 -D_XPG5 \
                 -D_REENTRANT \
                 -fPIC
