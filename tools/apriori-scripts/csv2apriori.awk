#!/bin/awk -f
#
# Converts an openoffice CSV (separated by commas) to make it suitable to
# be used with the default apriori format (separated by tabs, no spaces, no
# empty fields.)
#
# 2009 Bogdano Arendartchuk <debogdano@gmail.com>
#


function cleanup(s)
{
	gsub(/ /, "_", s) # spaces converted to _
	gsub(/\"/, "", s) # quote marks stripped
	return s
}

BEGIN {
	FS = ","
	OFS = "\t"

	# if set, it will replace empty columns by their column name
	if (fix_empty == "no")
		fix_empty = 0
	else
		fix_empty="yes"

	# if set, a list of columns that must have their tag prepended
	if (tag) {
		split(tag, tags, ",")
		ntags = length(tags)
	}

	# if set, a list of columns that should be removed
	if (kill) {
		split(kill, kills, ",")
		nkills = length(kills)
	}
}
{
	if (NR == 1) {
		# collects column names
		for (i = 1; i <= NF; i++)
			cols[i] = cleanup($i)
		ncols = i - 1
	}
	else {
		line = ""
		for (i = 1; i <= ncols; i++) {
			if ($i == "" && fix_empty)
				$i = cols[i] "_empty"
			else {
				$i = cleanup($i)
				if (ntags)
					for (nt = 1; nt <= ntags; nt++)
						if (tags[nt] == i) {
							$i = cols[i] "_" $i
							break
						}
			}

			if (skip) {
				sep = ""
				skip = 0
			}
			else
				sep = OFS
			if (nkills)
				for (nk = 1; nk <= nkills; nk++)
					if (kills[nk] == i) {
						skip = 1
						break
					}
			if (!skip)
				line = line sep $i
		}
		print line
	}
}
