#!/bin/sh
# small script to test the output of "rle" 
#

rle=./rle
mkdir -p tests-output
for text in tests/*.txt; do
	textname=`basename $text`
	out=tests-output/$textname
	$rle -c $text $out
	if cmp -s $out ${text}.out; then
		echo "C $text	ok"
	else
		echo "C $text	FAILED"
		exit 1
	fi
	dout=tests-output/${textname}
	$rle -d $out $dout
	if cmp -s $dout $text; then
		echo "D $text	ok"
	else
		echo "D $text	FAILED"
	fi
done
