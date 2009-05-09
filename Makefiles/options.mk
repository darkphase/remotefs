############################################
# Build options for remotefs
############################################

# If you want IPv6 connectivity remove
# the '#' character before OPT_1

OPT_1 = -DWITH_IPV6

###############################################
# If you want support for hard and symbolic links
# remove the '#' character fbefore OPT_2

###############################################
# If you want support for SSL encryption
# remove the '#' character for both lines

#OPT_2 = -DWITH_SSL
#OPT_2_LD = $(LDFLAGS_SSL)

###############################################
# Support of exports listing

OPT_3 = -DWITH_EXPORTS_LIST

###############################################
# Support for POSIX ACL

#OPT_4 = -DWITH_ACL

###############################################
# Support for UGO
# this normally should be embedded, but OpenWrt is commonly single-user system
# so, since it is designed for use with OpenWrt, it should be useful to remove
# unneeded parts from OpenWrt build

OPT_5 = -DWITH_UGO

###############################################
# For router with low CPU speed, spend other
# some time for other applications

OPT_6 = -DWITH_PAUSE

###############################################
# Experimental scheduling for MacOS only
#
#OPT_7 = -DWITH_SCHEDULING

###############################################
# End of user configuration, don't change the
# following lines

CFLAGS_OPTS = $(CFLAGS) $(OPT_1) $(OPT_2) $(OPT_3) $(OPT_4) $(OPT_5) $(OPT_6) $(OPT_7)
LDFLAGS_OPTS = $(LDFLAGS) $(OPT_1_LD) $(OPT_2_LD) $(OPT_3_LD) $(OPT_4_LD) $(OPT_5_LD) $(OPT_6_LD) $(OPT_7_LD)

# Default entry for main Makefile
help:

