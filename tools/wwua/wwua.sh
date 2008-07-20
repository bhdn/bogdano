#!/bin/sh
#
# Crappy script to translate words from ukrainian to english, using the
# service provided by http://lingresua.tripod.com/cgi-bin/oluaen.pl.
#

BASE=http://lingresua.tripod.com/cgi-bin/oluaen.pl
WORD="$1"
REQWORD=`echo $WORD | iconv -t windows-1251`
REQURL="$BASE?Word=""$REQWORD""&UaEnBtn=1"

lynx -dump "$REQURL" \
	| sed -n '/Requested words:/,/__/ p'
