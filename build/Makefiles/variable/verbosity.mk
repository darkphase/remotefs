# gmake
INC_VERBOSE=$(shell build/Makefiles/variable/verbose.sh)
# Solaris, FreeBSD
INC_VERBOSE:sh=build/Makefiles/variable/verbose.sh

include $(INC_VERBOSE)
