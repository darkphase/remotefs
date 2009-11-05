#!/bin/sh
if [ -f custom.mk ]
then
   echo custom.mk
else
   echo build/Makefiles/variable/empty.mk
fi
exit 0

