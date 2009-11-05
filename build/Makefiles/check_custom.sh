#!/bin/sh
if [ -f custom.mk ]
then
   echo custom.mk
else
   echo build/Makefiles/empty.mk
fi
exit 0

