include build/Makefiles/Linux.mk

################################
# The executables
################################

TOOLCHAIN_ROOT = toolchains/toolchain-arm_gcc4.1.2
ARCH = arm

CC = "$(TOOLCHAIN_ROOT)/bin/arm-linux-uclibc-gcc"
AR = "$(TOOLCHAIN_ROOT)/bin/arm-linux-uclibc-ar"

