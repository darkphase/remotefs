# gmake
INC_CUSTOM=$(shell build/Makefiles/variable/check_custom.sh)
# Solaris, FreeBSD
INC_CUSTOM:sh=build/Makefiles/variable/check_custom.sh

include $(INC_CUSTOM)
