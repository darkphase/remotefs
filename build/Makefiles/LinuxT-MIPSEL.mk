include build/Makefiles/Linux.mk 

################################
# The executables
################################

EXPERIMENTAL = no
TOOLCHAIN_ROOT = toolchains/toolchain-mipsel_gcc3.4.6
ARCH = mipsel

CC = "$(TOOLCHAIN_ROOT)/bin/mipsel-linux-uclibc-gcc"
AR = "$(TOOLCHAIN_ROOT)/bin/mipsel-linux-uclibc-ar"

