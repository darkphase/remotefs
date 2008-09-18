############################################
# Build options for remotefs
############################################

OPT_1 =  # allow use of IPv6

OPT_2 =   # Build remotefs so that uid and gid on the server
                    # are returned. chmod() and chown() are also
                    # implemented if this option is set.

#OPT_3 =   # provide POSIX ACL, set also OPT_2 if you want
                    # this.

#OPT_4 = 

OPTS = $(OPT_1)  $(OPT_2)  $(OPT_3)  $(OPT_4)  $(OPT_5) 


all: release
