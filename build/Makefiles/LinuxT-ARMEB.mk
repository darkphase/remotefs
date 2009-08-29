include build/Makefiles/Linux.mk

################################
# The executables
################################

TOOLCHAIN_ROOT = toolchains/toolchain-armeb_gcc4.1.2
ARCH = armeb

CC = "$(TOOLCHAIN_ROOT)/bin/armeb-linux-uclibc-gcc"
AR = "$(TOOLCHAIN_ROOT)/bin/armeb-linux-uclibc-ar"

