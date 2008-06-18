# times.awk
#
# produce a report from the timing information of Fly8.log.
#
# Note: may be out of date :-)
#

/^Welcome/ {
	printf ("%s %s %s %s\n", $6, $7, $8, $9)
}

/^Time/ {
	i = index ($2,":")
	secs = substr ($2, 1, i-1) * 60 + substr ($2, i+1)
}

/^nFrames/ {
	printf ("\t%s", $2)
	fps = int($2/secs*100+50)/100;
}

/^Elapsed/ {
	printf ("\t%s", $2)
	Elapsed = $2
}

/^(Video)|(3D)|(2D)|(Simulate)/ {
	printf ("\t%s", $2)
}

/^PageFlip/ {
	printf ("\t%s", $2)
	Nett = Elapsed - $2
}

/^Balance/ {
	printf ("\t%s\t%s\t%s\n", $2, Nett "", fps "")
}

