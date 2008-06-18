# leak.awk
#
# read a Fly8 log and highlight bad/missing memory frees. You need to first
# compile FLy8 with -DMEM_DEBUG.
#

{
	if ($1 == "strdup" || $1 == "strfree") {
		str = $2
		next
	}

	if ($1 == "alloc") {
		if ("" == str)
			str = $2
		mem[$3] = str
	} else if ($1 == "free") {
		if ("" == mem[$3])
			printf ("no alloc: %s\n", $3)
		else
			delete mem[$NF]
	}
	str = ""
}

END {
	for (x in mem) {
		printf ("no free:  %s %s\n", x, mem[x])
		delete mem[x]
	}
}
