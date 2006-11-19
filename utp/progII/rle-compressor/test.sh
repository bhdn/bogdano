#!/bin/sh
# small script to test the output of "rle" 
#

rle=./rle
mkdir -p tests-output
for text in tests/*.txt; do
	textname=`basename $text`
	out=tests-output/$textname.out
	$rle -c $text $out
	if cmp $out ${text}.out; then
		status=FAILED
	else
		status=ok
	fi
	echo "C $text $out	$status"
	dout=tests-output/${textname}
	$rle -d $out $dout
	if cmp -s $dout $text; then
		echo "D $text	ok"
	else
		echo "D $text	FAILED"
	fi
done
