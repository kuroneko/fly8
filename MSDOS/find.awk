# ---------------------------------- find.awk -------------------------------

# This is part of the flight simulator 'fly8'.
# Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).

# Find an address in the link map. used when a divide-overflow is reported
# in the log file.
#

BEGIN {
	address = -1
	main = 0
	prevline = ""
	prevaddr = ""
}

address >= 0 {
	if ("Abs" == $2)
		next
	if (a2n($1) >= address) {
		printf("%s [+0x%x]\n", prevline, address - a2n(prevaddr))
		exit (0)
	}
	prevline = $0
	prevaddr = $1
}

address < 0 {
	if ($0 ~ "Publics by Value") {
		address = a2n(AD)
		if (address < 0) {
			print "Bad address " AD
			exit (1)
		}
		address += main
	} else if ($2 == "_main")
		main = a2n($1) - a2n(MAIN)
}

function x2n(c, t) {
	t = index("0123456789ABCDEF", c) - 1
	if (t < 0)
		t = index("0123456789abcdef", c) - 1
	return t
}

function a2n(x, n,m,t,lx) {
	lx = length(x)
	n = 0
	m = 0
	for (i = 1; i <= lx; ++i) {
		t = substr(x,i,1)
		if (t == ":") {
			n = n*16 + m
			m = 0
			continue
		}
		t = x2n(t)
		if (t < 0)
			return -1
		m = m*16 + t
	}
	n = n*16 + m
	return n
}
