#!/bin/awk -f
# 
# It will discretize the values of a given column from a apriori-friendly
# text file. They are grouped by a number of slices of a given range (the
# domain of the column attribute).
#
# Required var parameters:
#
# - col: the number of the column to be modified
# - range: comma-separated pair of values of the M..N range. ie. the
#          attribute domain.
# - slices: the number of partitions of the range that will be created.
#
# 2009 Bogdano Arendartchuk <debogdano@gmail.com>
# 

BEGIN {
	FS="\t"
	OFS="\t"

	if (!col) {
		print "the parameter 'col' is mandatory"
		exit(1)
	}

	if (!range) {
		print "the parameter 'range' is mandatory"
		exit(1)
	}

	if (!slices) {
		print "the parameter 'slices' is mandatory"
		exit(1)
	}

	split(range, ranges, ",")
	rangem = ranges[1]
	rangen = ranges[2]
	delta = int(rangen - rangem)
	slice = int(delta / slices)
	delete ranges
}

{
	if (NR == 1)
		ncols = NF
	else {
		$1=$1

		for (i = 1; i <= ncols; i++) {
			if (i == col) {
				d = (int($i) - rangem)
				s = int(d / slice)
				if (d < slice)
					$i = "gte(" rangem ",B)"
				else if (s == slices)
					$i = "gte(" (rangen - slice) ",T)" 
				else
					$i = "gte(" (rangem + slice * s) ")"
			}
		}

		print
	}
}
