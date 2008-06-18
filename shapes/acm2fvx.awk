# --------------------------------- acm2fvx.awk ------------------------------

#
# This is part of the flight simulator 'fly8'.
# Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
#

#
# Convert an acm object description into the fly8 .fvx format. To use the
# result, e.g. f16.acm, just rename it to plane.vxx and do a make. This will
# generate the plane.vxx which is loaded at execution time.
#

BEGIN	{
	name = ARGV[1]
	acm = name ".acm"
	fly8 = name ".c"

	if (getline <acm <= 0) {
		print "could not read line 1 in " name
		exit 1
	}
	objname = $0

	if (getline <acm <= 0) {
		print "could not read line 2 in " name
		exit 1
	}
	npoints = $1+0
	npolys  = $2+0

	for (i = 1; i <= npoints; ++i) {
		if (getline <acm <= 0) {
			print "input error: point"
			exit 1
		}
		if ($1+0 != i) {
			print "input error: point number"
			exit 1
		}
		points[i,0] = round($3/3*16)	# x
		points[i,1] = round($2/3*16)	# y
		points[i,2] = round(-$4/3*16)	# z
	}

	printf ("#include \"shape.h\"\n") >fly8
	printf ("static VERTEX acm_%s[] = {\n", name) >>fly8

	for (i = 1; i <= npolys; ++i) {
		if (getline <acm <= 0) {
			print "input error: poly"
			exit 1
		}
		color = $1+0
		nedges = $2+0
		prev = 0;
		outpoint(prev, $3+0, 1)
		for (j = 1; j < nedges; ++j) {
			outpoint(prev, $(j+3), 0)
			prev = $(j+3)
		}
		outpoint(prev, $3+0, 0)
		printf ("\n") >>fly8
	}
	printf ("\t{{0, 0, 0}, V_EOF}\n};\n") >>fly8

	close (acm)
	close (fly8)
}

function outpoint(prev, p, type) {
	if (type)
		mode = "V_MOVE"
	else if ((prev, p) in edge || (p, prev) in edge)
		mode = "V_DUP"
	else {
		mode = "V_DRAW"
		edge[prev, p] = 1
	}
	printf ("\t{{%d, %d, %d}, %s},\n", \
		points[p,0], points[p,1], points[p,2], mode) >>fly8
}

function round(f) {
	if (f <= 0)
		return int(f-0.5)
	else
		return int(f+0.5)
}
