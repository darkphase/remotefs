############################################
# Build options for remotefs
############################################

###############################################
# IPv6 support

OPT_1 = -DWITH_IPV6

###############################################
# SSL support

#OPT_2 = -DWITH_SSL
#OPT_2_LD = $(LDFLAGS_SSL)

###############################################
# Support of exports listing

OPT_3 = -DWITH_EXPORTS_LIST

###############################################
# Support for POSIX ACL
# Linux only

#OPT_4 = -DWITH_ACL
#OPT_4_LD = $(LDFLAGS_ACL)

###############################################
# Support for UGO
# this normally should be embedded, but OpenWrt is commonly single-user system
# so, since it is designed for use with OpenWrt, it should be useful to remove
# unneeded parts from OpenWrt build

OPT_5 = -DWITH_UGO

###############################################
# Experimental scheduling for MacOS only

OPT_6 = -DWITH_SCHEDULING

###############################################
# End of user configuration, don't change the
# following lines

CFLAGS_OPTS = $(CFLAGS) $(OPT_1) $(OPT_2) $(OPT_3) $(OPT_4) $(OPT_5) $(OPT_6) $(OPT_7) $(OPT_8) $(OPT_9) $(OPT_10)
LDFLAGS_OPTS = $(LDFLAGS) $(OPT_1_LD) $(OPT_2_LD) $(OPT_3_LD) $(OPT_4_LD) $(OPT_5_LD) $(OPT_6_LD) $(OPT_7_LD) $(OPT_8_LD) $(OPT_9_LD) $(OPT_10_LD)

# Default entry for main Makefile
help:

