include Makefiles/Linux.mk

################################
# The executables
################################

TOOLCHAIN_ROOT = toolchains/toolchain-powerpc_gcc4.2.0
ARCH = powerpc

CC = "$(TOOLCHAIN_ROOT)/bin/powerpc-linux-uclibc-gcc"
AR = "$(TOOLCHAIN_ROOT)/bin/powerpc-linux-uclibc-ar"

