#!/bin/sh
# AUTO-GENERATED FILE, DO NOT EDIT!
if [ -f $1.org ]; then
  sed -e 's!^D:/Program/Cygwin/lib!/usr/lib!ig;s! D:/Program/Cygwin/lib! /usr/lib!ig;s!^D:/Program/Cygwin/bin!/usr/bin!ig;s! D:/Program/Cygwin/bin! /usr/bin!ig;s!^D:/Program/Cygwin/!/!ig;s! D:/Program/Cygwin/! /!ig;s!^F:!/cygdrive/f!ig;s! F:! /cygdrive/f!ig;s!^E:!/cygdrive/e!ig;s! E:! /cygdrive/e!ig;s!^D:!/cygdrive/d!ig;s! D:! /cygdrive/d!ig;s!^C:!/cygdrive/c!ig;s! C:! /cygdrive/c!ig;' $1.org > $1 && rm -f $1.org
fi
