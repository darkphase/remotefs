#!/bin/sh
case $V in
99)
echo build/Makefiles/variable/verbose.mk
;;
*)
echo build/Makefiles/variable/nonverbose.mk
;;
esac
exit 0
