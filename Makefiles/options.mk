############################################
# Build options for remotefs
############################################

# If you want IPv6 connectivity remove
# the '#' character before OPT_1

#OPT_1 = -DWITH_IPV6

###############################################
# If you want support for hard and symbolic links
# remove the '#' character fbefore OPT_2

#OPT_2 = -DWITH_LINKS

###############################################
# If you want support for SSL encryption
# remove the '#' character for both lines

#OPT_3 = -DWITH_SSL
#OPT_3_LD = $(LDFLAGS_SSL)

###############################################
# Support of exports listing

#OPT_4 = -DWITH_EXPORTS_LIST

###############################################
# Support for POSIX ACL

#OPT_5 = -DWITH_ACL

###############################################
# End of user configuration, don't change the
# following lines

CFLAGS_OPTS = $(CFLAGS) $(OPT_1) $(OPT_2) $(OPT_3) $(OPT_4) $(OPT_5)
LDFLAGS_OPTS = $(LDFLAGS) $(OPT_1_LD) $(OPT_2_LD) $(OPT_3_LD) $(OPT_4_LD) $(OPT_5_LD)

# Default entry for main Makefile
help:

