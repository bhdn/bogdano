#!/bin/sh -e
# this script applies all patches from a mandriva kernel tree into a linux
# git tree, no commit messsages or authors are preserved

PATCHESDIR=$1
if [ -z "$PATCHESDIR" ]; then
	echo "you must provide the patches/ directory" >&2
	exit 1
fi

LOGFILE=../imported-hash-`basename $(pwd)`
touch ${LOGFILE}
ORDER=`ls ${PATCHESDIR}*.patch ${PATCHESDIR}*.tar`
for path in ${ORDER}; do 
	name=`basename $path`
	chksum=`sha1sum $path | awk '{ print $1 }'`
	grep -q ${chksum} ${LOGFILE} && continue # skip, already imported
	case $path in
		*.tar) 
			set -x
			L=`tar xvf $path`
			F=`for l in $L; do test ! -d $l && echo $l; done`
			git-add -f -- $F
			git-commit -m "`basename $path`" -- $F
			cowsay "Extracted $name!"
		;;
		*.patch)
			patch -p1 < $path;
			K=`git-apply --numstat $path | awk '{ print $3 }' | uniq`
			X=`for line in $K; do test -f $line && echo $line; done` || :
			test -n "$X" && git-add -f $X
			git-commit -m "`basename $path`" $K
		;;
	esac
	echo $chksum >> ${LOGFILE}
done
