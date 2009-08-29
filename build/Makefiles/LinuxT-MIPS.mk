include build/Makefiles/Linux.mk

################################
# The executables
################################

TOOLCHAIN_ROOT = toolchains/toolchain-mips_gcc4.1.2
ARCH = mips

CC = "$(TOOLCHAIN_ROOT)/bin/mips-linux-uclibc-gcc"
AR = "$(TOOLCHAIN_ROOT)/bin/mips-linux-uclibc-ar"

